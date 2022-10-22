#pragma once

#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

#include "OCR_Filter.h"
#include "Video.h"
#include "FramerateController.h"
#include "BlockingQueue.h"
#include "CryptoManager.h"

#include <opencv2/opencv.hpp>

#include <gst/gst.h>
#include <glib.h>
#include "gstnvdsmeta.h"

class OCR_USDot : public OCR_Filter
{
public:

    OCR_USDot();
    ~OCR_USDot() override;

    // Initialize filter.
    // @param args Initialization arguments.
    bool InitRecognizer() override;

    static gboolean bus_call (GstBus * bus, GstMessage * msg, gpointer data);
    static GstPadProbeReturn pgie_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer ctx);
    static GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);

protected:

private:

    // Get analytic event type
    // @return License plate analytic type
    virtual AnalyticEvent::AnalyticType GetDefaultObjectType() override
    {
        return AnalyticEvent::AnalyticType::PlaneTailNumber;
    }

    // Detect text in an image, then pass detected text to 'DoTextRecognize' where it is recognized
    // @param inputImg Image on which to detect text in BGR pixel format, can be a combination of detected objects
    // @param originalImages Unmodified (e.g. no color inversion) images that should be used for events
    // @param locations Location of top left point of original image within inputImg
    // @param timestamp Timestamp of input frame
    // @param tryInverted Try text detection on color inverted image if no results found on original
    // @param ignoreResult Ignore any results from this input, used when processing stock image for engine initialization
    void DoTextDetect(cv::Mat &inputImg, std::vector<cv::Mat> &originalImages, std::vector<cv::Point> &locations,
                      const JDL::DateTime &timestamp, bool tryInverted, bool ignoreResult) override;


    // bool set_tracker_properties (GstElement *nvtracker);
    // gchar * get_absolute_file_path (gchar *cfg_file_path, gchar *file_path);
    void readConfig();
    void GetOutputThread();

    bool m_enable = false;
    bool m_enableDebugRender = false;
    std::thread m_runThread;
    std::thread m_outputThread;
    
    std::chrono::steady_clock::time_point m_start;
    GMainLoop *m_gstLoop = NULL;
    std::atomic_bool m_stop { false };

    GstElement *pipeline = NULL, *source = NULL,
        *streammux = NULL,  *pgie = NULL,
        *nvvidconv = NULL, *nvosd = NULL,
        *queue_sink = NULL, *nvvidconv_sink = NULL, *sink = NULL;

    GstBus *bus = NULL;
    guint bus_watch_id;
    GstPad *osd_sink_pad = NULL;


  #ifdef TRANSCODE_VIDEO
    GstElement *h264parser = NULL, *decoder = NULL,
               *encoder = NULL, *h264parser_sink = NULL;
  #else
    GstElement *nvvidconv_src = NULL, *caps_filter_src = NULL,
               *caps_filter_sink = NULL;
  #endif

  #ifdef PLATFORM_TEGRA
    GstElement *transform = NULL;
  #endif

    typedef struct {
        cv::Mat mat;
        JDL::DateTime timestamp;
        bool ignoreResult;
        int32_t frameWidth;
        int32_t frameHeight;
    } InputImage;
    InputImage buffInputImage;
};
