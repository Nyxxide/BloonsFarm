#ifndef BLOONSFARM_TEMPLATEMATCH_H
#define BLOONSFARM_TEMPLATEMATCH_H

#include "ScreenGrabber.h"
#include <opencv2/opencv.hpp>
#include <optional>
#include <array>
#include <unordered_map>
#include <thread>
#include <iostream>

/* ── tiny 64-bit hash (FNV-1a) ──────────────────────────────────────── */
static inline std::uint64_t tiny_hash64(const void* d,size_t n)
{
    const std::uint8_t* p=static_cast<const std::uint8_t*>(d);
    std::uint64_t h=14695981039346656037ULL;
    while(n--){ h^=*p++; h*=1099511628211ULL; }
    return h;
}

/* ── caches keyed by strong hash + scale ────────────────────────────── */
namespace tm_i {

    struct Key{ std::size_t id; double sc; };
    struct KeyHash{ std::size_t operator()(const Key&k)const noexcept{
            return k.id ^ std::hash<double>{}(k.sc);} };
    inline bool operator==(const Key&a,const Key&b) noexcept
    { return a.id==b.id && a.sc==b.sc; }

    inline thread_local std::unordered_map<Key,cv::Mat,KeyHash> gGray, gEdge;

    inline std::size_t strongId(const cv::Mat& im){
        return tiny_hash64(im.data, im.total()*im.elemSize())
               ^ (static_cast<std::size_t>(im.rows)<<16)
               ^  static_cast<std::size_t>(im.cols);
    }

    inline const cv::Mat& grayTpl(const cv::Mat& rgba,double sc){
        Key k{strongId(rgba),sc};
        auto it=gGray.find(k); if(it!=gGray.end()) return it->second;
        cv::Mat g,rs; cv::cvtColor(rgba,g,cv::COLOR_BGRA2GRAY);
        if(sc!=1.0) cv::resize(g,rs,{},sc,sc,cv::INTER_AREA); else rs=g;
        return gGray.emplace(k,std::move(rs)).first->second;
    }

    inline const cv::Mat& edgeTpl(const cv::Mat& rgba,double sc){
        Key k{strongId(rgba),sc};
        auto it=gEdge.find(k); if(it!=gEdge.end()) return it->second;
        const cv::Mat& g=grayTpl(rgba,sc);
        cv::Mat e; cv::Canny(g,e,60,120);
        return gEdge.emplace(k,std::move(e)).first->second;
    }
} // ns

struct Match{ cv::Rect bbox; double score; };

/* ── locateOnScreen ─────────────────────────────────────────────────── */
inline std::optional<Match>
locateOnScreen(const cv::Mat& needleRGBA,
               double threshold  = 0.90,
               int    referenceH = 1080,
               double edgeThresh = 0.80)
{
    /* grab once */
    cv::Mat hayRGBA = ScreenGrabber::grabScreen();
    if(hayRGBA.empty()||needleRGBA.empty()) return std::nullopt;
    cv::Mat hayGray; cv::cvtColor(hayRGBA,hayGray,cv::COLOR_BGRA2GRAY);

    /* coarse 4×4 pyramid */
    const int STRIDE=4;
    cv::Mat haySmall;
    cv::resize(hayGray,haySmall,
               {hayGray.cols/STRIDE,hayGray.rows/STRIDE},0,0,cv::INTER_AREA);

    constexpr std::array<double,5> REL={0.90,0.95,1.00,1.05,1.10};
    double dpi=static_cast<double>(hayGray.rows)/referenceH;

    Match best{{},-1.0}; double bestScale=1.0;
    for(double rel:REL){
        double scS=dpi*rel/STRIDE, scF=dpi*rel;
        const cv::Mat& tplS=tm_i::grayTpl(needleRGBA,scS);
        if(tplS.cols>haySmall.cols||tplS.rows>haySmall.rows) continue;
        cv::Mat res; cv::matchTemplate(haySmall,tplS,res,cv::TM_CCOEFF_NORMED);
        double v; cv::Point p; cv::minMaxLoc(res,nullptr,&v,nullptr,&p);
        if(v>best.score){ best={{p.x*STRIDE,p.y*STRIDE,
                                 tplS.cols*STRIDE,tplS.rows*STRIDE},v};
            bestScale=scF; if(v>=0.97) break;}
    }

    /* refine around coarse winner */
    if(best.score>=threshold){
        const cv::Mat& tplFull=tm_i::grayTpl(needleRGBA,bestScale);
        cv::Rect roi=best.bbox; roi.x=std::max(roi.x-4,0);
        roi.y=std::max(roi.y-4,0);
        roi.width =std::min(roi.width +8,hayGray.cols-roi.x);
        roi.height=std::min(roi.height+8,hayGray.rows-roi.y);

        cv::Mat fine; cv::matchTemplate(hayGray(roi),tplFull,fine,cv::TM_CCOEFF_NORMED);
        double fv; cv::Point fl; cv::minMaxLoc(fine,nullptr,&fv,nullptr,&fl);
        if(fv>=threshold)
            return Match{{roi.x+fl.x,roi.y+fl.y,tplFull.cols,tplFull.rows},fv};
    }

    /* skip edge if coarse was hopeless (<0.10) */
    if(best.score < 0.10) return std::nullopt;

    /* tight ROI for edge fallback (coarse ±12 px) */
    cv::Rect eROI = best.bbox;
    eROI.x = std::max(eROI.x-12,0);
    eROI.y = std::max(eROI.y-12,0);
    eROI.width  = std::min(eROI.width +24, hayGray.cols - eROI.x);
    eROI.height = std::min(eROI.height+24, hayGray.rows - eROI.y);

    /* build edges once */
    cv::Mat hayEdgeROI; cv::Canny(hayGray(eROI),hayEdgeROI,60,120);
    const cv::Mat& tplEdge = tm_i::edgeTpl(needleRGBA,bestScale);

    if(tplEdge.cols>hayEdgeROI.cols||tplEdge.rows>hayEdgeROI.rows)
        return std::nullopt;

    cv::Mat eRes;
    cv::matchTemplate(hayEdgeROI,tplEdge,eRes,cv::TM_SQDIFF_NORMED);
    double inv; cv::Point ep; cv::minMaxLoc(eRes,&inv,nullptr,&ep,nullptr);
    double eScore = 1.0 - inv;

    if(eScore>=edgeThresh)
        return Match{{eROI.x+ep.x,eROI.y+ep.y,tplEdge.cols,tplEdge.rows},eScore};

    return std::nullopt;
}

#endif /* BLOONSFARM_TEMPLATEMATCH_H */
