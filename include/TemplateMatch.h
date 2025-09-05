#ifndef BLOONSFARM_TEMPLATEMATCH_H
#define BLOONSFARM_TEMPLATEMATCH_H

#include "ScreenGrabber.h"
#include <opencv2/opencv.hpp>
#include <optional>
#include <array>
#include <unordered_map>
#include <thread>
#include <iostream>
#include <vector>
#include <cmath>

/* ── tiny 64-bit hash (FNV-1a) ───────────────────────────────────────── */
static inline std::uint64_t tiny_hash64(const void* d, std::size_t n)
{
    const auto* p = static_cast<const std::uint8_t*>(d);
    std::uint64_t h = 14695981039346656037ULL;
    while (n--) { h ^= *p++; h *= 1099511628211ULL; }
    return h;
}

/* ── channel-safe grayscale helper ───────────────────────────────────── */
static inline cv::Mat toGray(const cv::Mat& img)
{
    cv::Mat g;
    switch (img.channels()) {
        case 4: cv::cvtColor(img, g, cv::COLOR_BGRA2GRAY); break;
        case 3: cv::cvtColor(img, g, cv::COLOR_BGR2GRAY);  break;
        case 1: g = img;                                   break;
        default: cv::cvtColor(img, g, cv::COLOR_BGR2GRAY); break;
    }
    return g;
}

/* ── caches keyed by strong hash + scale ─────────────────────────────── */
namespace tm_i {

    struct Key       { std::size_t id; double sc; };
    struct KeyHasher {
        std::size_t operator()(const Key& k) const noexcept {
            return k.id ^ std::hash<double>{}(k.sc);
        }
    };
    inline bool operator==(const Key& a, const Key& b) noexcept
    { return a.id == b.id && a.sc == b.sc; }

    inline thread_local std::unordered_map<Key, cv::Mat, KeyHasher> gGray;
    inline thread_local std::unordered_map<Key, cv::Mat, KeyHasher> gEdge;

    inline std::size_t strongId(const cv::Mat& im)
    {
        return tiny_hash64(im.data, im.total() * im.elemSize())
               ^ (static_cast<std::size_t>(im.rows) << 16)
               ^  static_cast<std::size_t>(im.cols);
    }

    /* -------- pre-processed gray template (no CLAHE) ------------------ */
    inline const cv::Mat& grayTpl(const cv::Mat& tplAny, double sc)
    {
        Key k{ strongId(tplAny), sc };
        if (auto it = gGray.find(k); it != gGray.end()) return it->second;

        cv::Mat g = toGray(tplAny);
        cv::Mat rs;
        if (sc != 1.0)
            cv::resize(g, rs, {}, sc, sc, cv::INTER_AREA);
        else
            rs = g;

        return gGray.emplace(k, std::move(rs)).first->second;
    }

    /* -------- Canny edge template (built on top of gray) -------------- */
    inline const cv::Mat& edgeTpl(const cv::Mat& tplAny, double sc)
    {
        Key k{ strongId(tplAny), sc };
        if (auto it = gEdge.find(k); it != gEdge.end()) return it->second;

        const cv::Mat& g = grayTpl(tplAny, sc);
        cv::Mat e;  cv::Canny(g, e, 60, 120);
        return gEdge.emplace(k, std::move(e)).first->second;
    }
} // namespace tm_i

/* ── macOS helpers: capture ALL displays, normalize Retina to 1× ─────── */
#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>

  static inline cv::Mat mac_captureDisplayBGR1x(CGDirectDisplayID display)
  {
      CGImageRef img = CGDisplayCreateImage(display);
      if (!img) return {};

      const size_t wpx = CGImageGetWidth(img);
      const size_t hpx = CGImageGetHeight(img);
      if (wpx == 0 || hpx == 0) { CGImageRelease(img); return {}; }

      cv::Mat bgra((int)hpx, (int)wpx, CV_8UC4);

      CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
      if (!cs) { CGImageRelease(img); return {}; }

      CGContextRef ctx = CGBitmapContextCreate(
          bgra.data, wpx, hpx, 8, (size_t)bgra.step[0], cs,
          kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
      if (!ctx) {
          CGColorSpaceRelease(cs);
          CGImageRelease(img);
          return {};
      }

      CGContextDrawImage(ctx, CGRectMake(0, 0, (CGFloat)wpx, (CGFloat)hpx), img);
      CGContextRelease(ctx);
      CGColorSpaceRelease(cs);
      CGImageRelease(img);

      // Normalize Retina (backing pixels) → logical points (1×)
      CGRect bounds = CGDisplayBounds(display);
      const double pw = (double)CGRectGetWidth(bounds);
      const double ph = (double)CGRectGetHeight(bounds);
      double scale = 1.0;
      if (pw > 0.0 && ph > 0.0) {
          const double sx = (double)wpx / pw;
          const double sy = (double)hpx / ph;
          scale = std::max(sx, sy); // usually 2.0 on Retina
      }
      if (scale > 1.01) {
          cv::Mat tmp;
          cv::resize(bgra, tmp,
                     cv::Size((int)std::lround(bgra.cols / scale),
                              (int)std::lround(bgra.rows / scale)),
                     0, 0, cv::INTER_AREA);
          bgra = std::move(tmp);
      }

      // Return BGR (match your pipeline)
      cv::Mat bgr;
      cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
      return bgr;
  }

  static inline std::vector<cv::Mat> mac_captureAllDisplaysBGR1x()
  {
      uint32_t count = 0;
      CGGetActiveDisplayList(0, nullptr, &count);
      std::vector<CGDirectDisplayID> ids(count);
      if (count) CGGetActiveDisplayList(count, ids.data(), &count);

      std::vector<cv::Mat> shots;
      shots.reserve(count ? count : 1);
      if (count == 0) {
          // Fallback to main display
          shots.emplace_back(mac_captureDisplayBGR1x(kCGDirectMainDisplay));
          return shots;
      }
      for (auto d : ids) {
          shots.emplace_back(mac_captureDisplayBGR1x(d));
      }
      return shots;
  }
#endif

/* --------------------------------------------------------------------- */
struct Match { cv::Rect bbox; double score; };

/* ── locateOnScreen ──────────────────────────────────────────────────── */
inline std::optional<Match>
locateOnScreen(const cv::Mat& needleRGBA,
               double threshold  = 0.90,
               int    referenceH = 1080,
               double edgeThresh = 0.80)
{
    // Build the list of "haystack" screenshots:
    std::vector<cv::Mat> hayBGRs;
#if defined(__APPLE__)
    hayBGRs = mac_captureAllDisplaysBGR1x();   // all displays, 1× normalized
#else
    {
        cv::Mat single = ScreenGrabber::grabScreen(); // existing Windows/Linux path
        if (!single.empty()) hayBGRs.push_back(std::move(single));
    }
#endif

    if (hayBGRs.empty() || needleRGBA.empty())
        return std::nullopt;

    // Precompute template scales per call
    constexpr int STRIDE = 4;
    constexpr std::array<double, 5> REL = { 0.90, 0.95, 1.00, 1.05, 1.10 };

    for (const cv::Mat& hayBGR : hayBGRs)
    {
        // ❶ grayscale hay (channel-safe)
        cv::Mat hayGray = toGray(hayBGR);
        if (hayGray.empty()) continue;

        // ❷ pyramid downsample by STRIDE
        cv::Mat haySmall;
        cv::resize(hayGray, haySmall,
                   { hayGray.cols / STRIDE, hayGray.rows / STRIDE },
                   0, 0, cv::INTER_AREA);

        // dpi factor vs reference height (1080)
        double dpi = static_cast<double>(hayGray.rows) / std::max(1, referenceH);

        Match  best      { {}, -1.0 };
        double bestScale = 1.0;

        // ❸ coarse search at several relative scales
        for (double rel : REL)
        {
            double scS = dpi * rel / STRIDE;   // scale used for small pyramid
            double scF = dpi * rel;            // full-res scale

            const cv::Mat& tplS = tm_i::grayTpl(needleRGBA, scS);
            if (tplS.empty() || tplS.cols > haySmall.cols || tplS.rows > haySmall.rows)
                continue;

            cv::Mat res;
            cv::matchTemplate(haySmall, tplS, res, cv::TM_CCOEFF_NORMED);

            double v;  cv::Point p;
            cv::minMaxLoc(res, nullptr, &v, nullptr, &p);

            if (v > best.score) {
                best = { { p.x * STRIDE, p.y * STRIDE,
                           tplS.cols * STRIDE, tplS.rows * STRIDE }, v };
                bestScale = scF;
                if (v >= 0.97) break; // early exit
            }
        }

        // ❹ refine around coarse winner – adaptive ROI growth
        if (best.score >= 0.10)
        {
            int grow = (best.score < 0.60) ? 30 : 8;

            cv::Rect roi = best.bbox;
            roi.x = std::max(roi.x - grow, 0);
            roi.y = std::max(roi.y - grow, 0);
            roi.width  = std::min(roi.width  + 2 * grow, hayGray.cols - roi.x);
            roi.height = std::min(roi.height + 2 * grow, hayGray.rows - roi.y);

            const cv::Mat& tplFull = tm_i::grayTpl(needleRGBA, bestScale);

            if (!tplFull.empty() &&
                tplFull.cols <= roi.width && tplFull.rows <= roi.height)
            {
                cv::Mat fine;
                cv::matchTemplate(hayGray(roi), tplFull, fine, cv::TM_CCOEFF_NORMED);

                double fv;  cv::Point fl;
                cv::minMaxLoc(fine, nullptr, &fv, nullptr, &fl);

                if (fv >= threshold)
                    return Match{ { roi.x + fl.x, roi.y + fl.y,
                                    tplFull.cols,  tplFull.rows }, fv };
            }
        }

        // ❺ edge fallback (only if we had any coarse signal)
        if (best.score >= 0.10)
        {
            cv::Rect eROI = best.bbox;
            eROI.x = std::max(eROI.x - 12, 0);
            eROI.y = std::max(eROI.y - 12, 0);
            eROI.width  = std::min(eROI.width  + 24, hayGray.cols - eROI.x);
            eROI.height = std::min(eROI.height + 24, hayGray.rows - eROI.y);

            cv::Mat hayEdgeROI;
            cv::Canny(hayGray(eROI), hayEdgeROI, 60, 120);

            const cv::Mat& tplEdge = tm_i::edgeTpl(needleRGBA, bestScale);
            if (!tplEdge.empty() &&
                tplEdge.cols <= eROI.width && tplEdge.rows <= eROI.height)
            {
                cv::Mat eRes;
                cv::matchTemplate(hayEdgeROI, tplEdge, eRes, cv::TM_SQDIFF_NORMED);

                double inv;  cv::Point ep;
                cv::minMaxLoc(eRes, &inv, nullptr, &ep, nullptr);
                double eScore = 1.0 - inv;

                if (eScore >= edgeThresh)
                    return Match{ { eROI.x + ep.x, eROI.y + ep.y,
                                    tplEdge.cols,  tplEdge.rows }, eScore };
            }
        }

        // No hit on this display → try next (macOS may have more displays)
    }

    // No matches on any display
    return std::nullopt;
}

#endif /* BLOONSFARM_TEMPLATEMATCH_H */
