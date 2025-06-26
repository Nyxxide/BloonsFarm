#ifndef BLOONSFARM_TEMPLATEMATCH_H
#define BLOONSFARM_TEMPLATEMATCH_H

#include "ScreenGrabber.h"
#include <opencv2/opencv.hpp>
#include <optional>
#include <array>
#include <algorithm>

struct Match { cv::Rect bbox; double score; };

/* ── helpers ─────────────────────────────────────────────────────────────── */
inline cv::Mat grayNoAlpha(const cv::Mat& rgba)
{
    cv::Mat rgb, g;
    cv::cvtColor(rgba, rgb,  cv::COLOR_BGRA2BGR);
    cv::cvtColor(rgb,  g,    cv::COLOR_BGR2GRAY);
    return g;
}
inline cv::Mat bgrNoAlpha(const cv::Mat& rgba)
{
    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}
inline cv::Size shrink(const cv::Size& s, int stride)
{
    return { (s.width  + stride-1) / stride,
             (s.height + stride-1) / stride };
}

/* ── locateOnScreen (grayscale first, colour fallback) ───────────────────── */
inline std::optional<Match>
locateOnScreen(const cv::Mat& needleRGBA,
               double threshold      = 0.85,     // gray high-threshold
               int    referenceH     = 1080,
               double colourFallback = 0.80)     // accept colour ≥ 0.80
{
    /* 1. Capture screen once ------------------------------------------------ */
    cv::Mat hayRGBA = ScreenGrabber::grabScreen();
    if (hayRGBA.empty() || needleRGBA.empty()) return std::nullopt;

    cv::Mat hayGrayFull = grayNoAlpha(hayRGBA);
    cv::Mat hayBGR      = bgrNoAlpha(hayRGBA);      // for colour fallback

    /* 2. DPI scaling -------------------------------------------------------- */
    double dpiScale = static_cast<double>(hayGrayFull.rows) / referenceH;

    constexpr std::array<double,7> rel = {0.85,0.90,0.95,1.00,1.05,1.10,1.15};
    const int STRIDE = 2;

    cv::Mat haySmall;
    cv::resize(hayGrayFull, haySmall, shrink(hayGrayFull.size(), STRIDE),
               0,0, cv::INTER_AREA);

    Match   best{{},-1.0};
    double  bestScale = 1.0;

    /* 3. Coarse gray multi-scale loop -------------------------------------- */
    for (double r : rel) {
        double scale = dpiScale * r;

        cv::Mat tplGray;
        cv::resize(grayNoAlpha(needleRGBA), tplGray, {}, scale, scale, cv::INTER_AREA);
        if (tplGray.cols > hayGrayFull.cols || tplGray.rows > hayGrayFull.rows) continue;

        cv::Mat tplSmall;
        cv::resize(tplGray, tplSmall, shrink(tplGray.size(), STRIDE), 0,0, cv::INTER_AREA);

        cv::Mat res;
        cv::matchTemplate(haySmall, tplSmall, res, cv::TM_CCOEFF_NORMED);

        double maxVal; cv::Point maxLoc;
        cv::minMaxLoc(res,nullptr,&maxVal,nullptr,&maxLoc);

        if (maxVal > best.score) {
            best       = { {maxLoc.x*STRIDE, maxLoc.y*STRIDE,
                            tplGray.cols, tplGray.rows}, maxVal };
            bestScale  = scale;
            if (maxVal >= 0.97) break;          // strong hit, stop early
        }
    }

    /* 4. If gray confident enough, refine ROI and return ------------------- */
    if (best.score >= threshold)
    {
        cv::Mat tplGray;
        cv::resize(grayNoAlpha(needleRGBA), tplGray, {}, bestScale, bestScale, cv::INTER_AREA);

        cv::Rect roi = best.bbox;
        roi.x      = std::max(roi.x - 4, 0);
        roi.y      = std::max(roi.y - 4, 0);
        roi.width  = std::min(roi.width  + 8, hayGrayFull.cols - roi.x);
        roi.height = std::min(roi.height + 8, hayGrayFull.rows - roi.y);

        cv::Mat fine;
        cv::matchTemplate(hayGrayFull(roi), tplGray, fine, cv::TM_CCOEFF_NORMED);

        double fineVal; cv::Point fineLoc;
        cv::minMaxLoc(fine,nullptr,&fineVal,nullptr,&fineLoc);

        return Match{ {roi.x+fineLoc.x, roi.y+fineLoc.y,
                       tplGray.cols, tplGray.rows}, fineVal };
    }

    /* 5. Colour fallback (single-scale, no stride) ------------------------- */
    cv::Mat tplBGR;
    cv::resize(bgrNoAlpha(needleRGBA), tplBGR, {}, bestScale, bestScale, cv::INTER_AREA);

    if (tplBGR.cols <= hayBGR.cols && tplBGR.rows <= hayBGR.rows)
    {
        cv::Mat res;
        cv::matchTemplate(hayBGR, tplBGR, res, cv::TM_CCOEFF_NORMED);

        double maxVal; cv::Point maxLoc;
        cv::minMaxLoc(res,nullptr,&maxVal,nullptr,&maxLoc);

        if (maxVal >= colourFallback)
            return Match{ {maxLoc, tplBGR.size()}, maxVal };
    }

    return std::nullopt;                          // nothing qualified
}

#endif /* BLOONSFARM_TEMPLATEMATCH_H */
