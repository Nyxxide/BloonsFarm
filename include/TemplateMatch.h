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

#if defined(__APPLE__)
    /* Build (gray, mask) pair for masked matching.
       - If template has alpha → use it as mask.
       - Else → build a mask from edges so JPEGs still benefit from masking. */
    inline void tplGrayAndMask(const cv::Mat& tplAny, double sc,
                               cv::Mat& outGray, cv::Mat& outMask /* may be empty */)
    {
        // Gray
        cv::Mat g = toGray(tplAny);
        if (sc != 1.0) cv::resize(g, g, {}, sc, sc, cv::INTER_AREA);

        // Prefer real alpha
        outMask.release();
        if (tplAny.channels() == 4) {
            cv::Mat a; cv::extractChannel(tplAny, a, 3);   // alpha
            if (sc != 1.0) cv::resize(a, a, g.size(), 0, 0, cv::INTER_AREA);
            cv::threshold(a, outMask, 10, 255, cv::THRESH_BINARY);
        } else {
            // Derive a conservative mask from edges (rejects background)
            cv::Mat e; cv::Canny(g, e, 50, 100);
            // thicken a bit to avoid ultra-thin masks
            cv::dilate(e, e, cv::getStructuringElement(cv::MORPH_ELLIPSE, {3,3}));
            // convert to 0/255
            outMask = e.clone();
        }
        outGray = std::move(g);
    }
#endif
} // namespace tm_i

/* --------------------------------------------------------------------- */
struct Match { cv::Rect bbox; double score; };

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>

  /* Capture one display at logical 1× and return BGR */
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
          shots.emplace_back(mac_captureDisplayBGR1x(kCGDirectMainDisplay));
          return shots;
      }
      for (auto d : ids) shots.emplace_back(mac_captureDisplayBGR1x(d));
      return shots;
  }

  /* suppress a neighborhood around a point in a response map (for 2nd peak) */
  static inline void suppressAround(cv::Mat& resp, cv::Point c, int rad)
  {
      int x0 = std::max(0, c.x - rad);
      int y0 = std::max(0, c.y - rad);
      int x1 = std::min(resp.cols, c.x + rad);
      int y1 = std::min(resp.rows, c.y + rad);
      resp(cv::Rect(x0, y0, x1 - x0, y1 - y0)).setTo(0); // for CCORR_NORMED
  }

  /* quick color sanity check (rejects look-alikes) */
  static inline bool passColorCheck(const cv::Mat& hayBGR,
                                    const cv::Mat& tplBGRScaled,
                                    const cv::Rect& at, double tol = 20.0 /* per channel */)
  {
      if (at.x < 0 || at.y < 0 || at.x + tplBGRScaled.cols > hayBGR.cols || at.y + tplBGRScaled.rows > hayBGR.rows)
          return false;
      cv::Mat roi = hayBGR(at);
      cv::Mat diff; cv::absdiff(roi, tplBGRScaled, diff);
      cv::Scalar m = cv::mean(diff);
      return (m[0] + m[1] + m[2]) / 3.0 <= tol;
  }

  /* gradient-shape correlation inside mask (illumination-robust) */
  static inline double maskedGradCorr(const cv::Mat& aGray, const cv::Mat& bGray, const cv::Mat& mask8u)
  {
      CV_Assert(aGray.size() == bGray.size());
      CV_Assert(aGray.type() == CV_8U && bGray.type() == CV_8U);

      cv::Mat ga, gb;
      cv::Sobel(aGray, ga, CV_32F, 1, 1, 3); // simple magnitude proxy
      cv::Sobel(bGray, gb, CV_32F, 1, 1, 3);

      // zero-mean inside mask
      cv::Scalar ma = cv::mean(ga, mask8u);
      cv::Scalar mb = cv::mean(gb, mask8u);
      ga -= (float)ma[0];
      gb -= (float)mb[0];

      // numerator
      cv::Mat prod; cv::multiply(ga, gb, prod);
      cv::Mat m32; mask8u.convertTo(m32, CV_32F, 1.0/255.0);
      cv::multiply(prod, m32, prod);
      double num = cv::sum(prod)[0];

      // denominator
      cv::Mat a2; cv::multiply(ga, ga, a2); cv::multiply(a2, m32, a2);
      cv::Mat b2; cv::multiply(gb, gb, b2); cv::multiply(b2, m32, b2);
      double den = std::sqrt(std::max(1e-12, cv::sum(a2)[0]) * std::max(1e-12, cv::sum(b2)[0]));

      return (den > 0.0) ? (num / den) : 0.0; // roughly [-1,1]
  }
#endif

/* ── locateOnScreen ──────────────────────────────────────────────────── */
inline std::optional<Match>
locateOnScreen(const cv::Mat& needleRGBA,
               double threshold  = 0.90,
               int    referenceH = 1080,
               double edgeThresh = 0.80)
{
#if defined(__APPLE__)
    // ---------- macOS path ----------
    std::vector<cv::Mat> hayBGRs = mac_captureAllDisplaysBGR1x();
    if (hayBGRs.empty() || needleRGBA.empty())
        return std::nullopt;

    // Template base (1×) reference height
    cv::Mat tplGray1x = toGray(needleRGBA);
    int tplBaseH = tplGray1x.rows;
    if (tplBaseH <= 0) return std::nullopt;

    for (const cv::Mat& hayBGR : hayBGRs)
    {
        if (hayBGR.empty()) continue;

        cv::Mat hayGray = toGray(hayBGR);
        if (hayGray.empty()) continue;

        // Center the scale around actual screen / template ratio
        double r = static_cast<double>(hayGray.rows) / std::max(1, tplBaseH);
        const std::array<double,7> MUL = {0.75, 0.85, 0.92, 1.00, 1.08, 1.15, 1.30};

        // Coarse search on 4× downsample
        constexpr int STRIDE = 4;
        cv::Mat haySmall;
        cv::resize(hayGray, haySmall,
                   { hayGray.cols / STRIDE, hayGray.rows / STRIDE },
                   0, 0, cv::INTER_AREA);

        Match  best      { {}, -1.0 };
        double bestScale = 1.0;

        for (double mul : MUL)
        {
            double scF = r * mul;             // full-res vs. base tpl
            double scS = scF / STRIDE;        // downsampled scale

            cv::Mat tplGray, tplMask;
            tm_i::tplGrayAndMask(needleRGBA, scS, tplGray, tplMask);
            if (tplGray.empty()) continue;
            if (tplGray.cols > haySmall.cols || tplGray.rows > haySmall.rows) continue;

            // Erode mask slightly to reduce halo influence
            cv::Mat tplMaskUse = tplMask;
            if (!tplMaskUse.empty())
                cv::erode(tplMaskUse, tplMaskUse, cv::getStructuringElement(cv::MORPH_ELLIPSE, {3,3}));

            cv::Mat res;
            if (!tplMaskUse.empty())
                cv::matchTemplate(haySmall, tplGray, res, cv::TM_CCORR_NORMED, tplMaskUse);
            else
                cv::matchTemplate(haySmall, tplGray, res, cv::TM_CCORR_NORMED);

            double v;  cv::Point p;
            cv::minMaxLoc(res, nullptr, &v, nullptr, &p);

            if (v > best.score) {
                best = { { p.x * STRIDE, p.y * STRIDE,
                           tplGray.cols * STRIDE, tplGray.rows * STRIDE }, v };
                bestScale = scF;
                if (v >= 0.990) break; // extremely strong
            }
        }

        if (best.score < 0.10) {
            // no signal on this display
            continue;
        }

        // Refine around winner on full-res
        int grow = (best.score < 0.60) ? 30 : 12;
        cv::Rect roi = best.bbox;
        roi.x = std::max(roi.x - grow, 0);
        roi.y = std::max(roi.y - grow, 0);
        roi.width  = std::min(roi.width  + 2 * grow, hayGray.cols - roi.x);
        roi.height = std::min(roi.height + 2 * grow, hayGray.rows - roi.y);

        cv::Mat tplFullGray, tplFullMask;
        tm_i::tplGrayAndMask(needleRGBA, bestScale, tplFullGray, tplFullMask);

        if (tplFullGray.empty() ||
            tplFullGray.cols > roi.width || tplFullGray.rows > roi.height)
        {
            goto MAC_EDGE_FALLBACK;
        }

        // Shrink mask a tad for final pass
        cv::Mat finalMask = tplFullMask;
        if (!finalMask.empty())
            cv::erode(finalMask, finalMask, cv::getStructuringElement(cv::MORPH_ELLIPSE, {3,3}));

        cv::Mat fine;
        if (!finalMask.empty())
            cv::matchTemplate(hayGray(roi), tplFullGray, fine, cv::TM_CCORR_NORMED, finalMask);
        else
            cv::matchTemplate(hayGray(roi), tplFullGray, fine, cv::TM_CCORR_NORMED);

        double top; cv::Point loc;
        cv::minMaxLoc(fine, nullptr, &top, nullptr, &loc);

        // Distinctness: top vs. second-best
        cv::Mat fine2 = fine.clone();
        int rad = std::max(2, std::min(tplFullGray.cols, tplFullGray.rows) / 8);
        suppressAround(fine2, loc, rad);
        double secondBest; cv::minMaxLoc(fine2, nullptr, &secondBest, nullptr, nullptr);

        bool okScore   = (top >= std::max(0.94, threshold));   // slightly stricter on mac
        bool okDistinct= (top - secondBest >= 0.03);

        // Build BGR template at bestScale for color sanity check
        cv::Mat tplBGR;
        if (needleRGBA.channels() == 4)
            cv::cvtColor(needleRGBA, tplBGR, cv::COLOR_BGRA2BGR);
        else if (needleRGBA.channels() == 3)
            tplBGR = needleRGBA;
        else
            cv::cvtColor(needleRGBA, tplBGR, cv::COLOR_GRAY2BGR);
        if (bestScale != 1.0)
            cv::resize(tplBGR, tplBGR, tplFullGray.size(), 0, 0, cv::INTER_AREA);

        cv::Rect found(roi.x + loc.x, roi.y + loc.y, tplFullGray.cols, tplFullGray.rows);
        bool okColor = passColorCheck(hayBGR, tplBGR, found, 20.0);

        // Gradient-shape correlation inside mask (illumination robust)
        double gCorr = (!finalMask.empty())
                       ? maskedGradCorr(hayGray(found), tplFullGray, finalMask)
                       : maskedGradCorr(hayGray(found), tplFullGray, cv::Mat::ones(tplFullGray.size(), CV_8U)*255);
        bool okGrad = (gCorr >= 0.35); // modest threshold; adjust if needed

        if (okScore && okDistinct && okColor && okGrad) {
            return Match{ found, top };
        }

MAC_EDGE_FALLBACK:
        {
            cv::Rect eROI = best.bbox;
            eROI.x = std::max(eROI.x - 12, 0);
            eROI.y = std::max(eROI.y - 12, 0);
            eROI.width  = std::min(eROI.width  + 24, hayGray.cols - eROI.x);
            eROI.height = std::min(eROI.height + 24, hayGray.rows - eROI.y);

            cv::Mat hayEdgeROI; cv::Canny(hayGray(eROI), hayEdgeROI, 60, 120);

            const cv::Mat& tplEdge = tm_i::edgeTpl(needleRGBA, bestScale);
            if (!tplEdge.empty() &&
                tplEdge.cols <= eROI.width && tplEdge.rows <= eROI.height)
            {
                cv::Mat eRes;
                cv::matchTemplate(hayEdgeROI, tplEdge, eRes, cv::TM_SQDIFF_NORMED);

                double inv;  cv::Point ep;
                cv::minMaxLoc(eRes, &inv, nullptr, &ep, nullptr);
                double eScore = 1.0 - inv;

                if (eScore >= std::max(0.87, edgeThresh)) {
                    cv::Rect f(eROI.x + ep.x, eROI.y + ep.y, tplEdge.cols, tplEdge.rows);
                    return Match{ f, eScore };
                }
            }
        }

        // try next display
    }

    return std::nullopt;

#else
    // -------- Windows/Linux (unchanged) --------
    cv::Mat hayRGBA = ScreenGrabber::grabScreen();
    if (hayRGBA.empty() || needleRGBA.empty()) return std::nullopt;

    cv::Mat hayGray;
    cv::cvtColor(hayRGBA, hayGray, cv::COLOR_BGRA2GRAY);

    constexpr int STRIDE = 4;
    cv::Mat haySmall;
    cv::resize(hayGray, haySmall,
               { hayGray.cols / STRIDE, hayGray.rows / STRIDE },
               0, 0, cv::INTER_AREA);

    constexpr std::array<double, 5> REL = { 0.90, 0.95, 1.00, 1.05, 1.10 };
    double dpi = static_cast<double>(hayGray.rows) / referenceH;

    Match  best      { {}, -1.0 };
    double bestScale = 1.0;

    for (double rel : REL)
    {
        double scS = dpi * rel / STRIDE;
        double scF = dpi * rel;

        const cv::Mat& tplS = tm_i::grayTpl(needleRGBA, scS);
        if (tplS.cols > haySmall.cols || tplS.rows > haySmall.rows) continue;

        cv::Mat res;
        cv::matchTemplate(haySmall, tplS, res, cv::TM_CCOEFF_NORMED);

        double v;  cv::Point p;
        cv::minMaxLoc(res, nullptr, &v, nullptr, &p);

        if (v > best.score) {
            best = { { p.x * STRIDE, p.y * STRIDE,
                       tplS.cols * STRIDE, tplS.rows * STRIDE }, v };
            bestScale = scF;
            if (v >= 0.97) break;
        }
    }

    if (best.score >= 0.10)
    {
        int grow = (best.score < 0.60) ? 30 : 8;

        cv::Rect roi = best.bbox;
        roi.x = std::max(roi.x - grow, 0);
        roi.y = std::max(roi.y - grow, 0);
        roi.width  = std::min(roi.width  + 2 * grow, hayGray.cols - roi.x);
        roi.height = std::min(roi.height + 2 * grow, hayGray.rows - roi.y);

        const cv::Mat& tplFull = tm_i::grayTpl(needleRGBA, bestScale);

        if (tplFull.cols <= roi.width && tplFull.rows <= roi.height) {
            cv::Mat fine;
            cv::matchTemplate(hayGray(roi), tplFull, fine, cv::TM_CCOEFF_NORMED);

            double fv;  cv::Point fl;
            cv::minMaxLoc(fine, nullptr, &fv, nullptr, &fl);

            if (fv >= threshold)
                return Match{ { roi.x + fl.x, roi.y + fl.y,
                                tplFull.cols,  tplFull.rows }, fv };
        }
    }

    if (best.score < 0.10) return std::nullopt;

    cv::Rect eROI = best.bbox;
    eROI.x = std::max(eROI.x - 12, 0);
    eROI.y = std::max(eROI.y - 12, 0);
    eROI.width  = std::min(eROI.width  + 24, hayGray.cols - eROI.x);
    eROI.height = std::min(eROI.height + 24, hayGray.rows - eROI.y);

    cv::Mat hayEdgeROI;
    cv::Canny(hayGray(eROI), hayEdgeROI, 60, 120);

    const cv::Mat& tplEdge = tm_i::edgeTpl(needleRGBA, bestScale);
    if (tplEdge.cols > hayEdgeROI.cols || tplEdge.rows > hayEdgeROI.rows)
        return std::nullopt;

    cv::Mat eRes;
    cv::matchTemplate(hayEdgeROI, tplEdge, eRes, cv::TM_SQDIFF_NORMED);

    double inv;  cv::Point ep;
    cv::minMaxLoc(eRes, &inv, nullptr, &ep, nullptr);
    double eScore = 1.0 - inv;

    if (eScore >= edgeThresh)
        return Match{ { eROI.x + ep.x, eROI.y + ep.y,
                        tplEdge.cols,  tplEdge.rows }, eScore };

    return std::nullopt;
#endif
}

#endif /* BLOONSFARM_TEMPLATEMATCH_H */
