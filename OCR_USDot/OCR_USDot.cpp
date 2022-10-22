#include "OCR_USDot.h"

#include <math.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <stdio.h>

#include "LogUtil.h"
#include "Events.h"
#include "LibInterfaceUtil.h"
#include "Buffer.h"


#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <cuda_runtime_api.h>
#include "gstnvdsmeta.h"
#include "nvbufsurface.h"
#include "nvds_obj_encode.h"
#include <curl/curl.h>
#define CONFIG_PATH "./Deepstream/USDot/deepstream_src_appsrc_app_config.txt"
#define PGIE_CONFIG_FILE "./Deepstream/USDot/src_pgie_config.txt"
#define MAX_DISPLAY_LEN 64

#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2

#define CUSTOM_PTS 1

gint frame_number = 0;

#define USDOT_SIZE 256
#define save_img TRUE
#define attach_user_meta TRUE

typedef struct {
    JDL::DateTime timestamp;
    uint64_t id;
    std::string text;
    int32_t confidence;
} USDotResult;
static std::deque<USDotResult> OCR_USDot_Result;

/*config vars*/
gint muxer_width = 0;
gint muxer_height = 0;
gint muxer_live_source = 0;
gint muxer_batch_size = 0;
gint muxer_batched_push_timeout = 0;
gint muxer_nvbuf_memory_type = 0;
gint open_rec = 0;
char fileNameString[30];
gint rec_min_width = 0;
gint rec_min_height = 0;

/* These are the strings of the labels for the respective models */
gchar pgie_classes_str[4][32] = {"Vehicle", "TwoWheeler", "Person", "RoadSign"};

NvDsObjEncCtxHandle obj_ctx_handle = nullptr;

OCR_USDot::OCR_USDot()
{
    struct stat info;
    bool isCreatedDir = false;
    if (stat("/opt/USDot", &info) != 0) {
        isCreatedDir = true;
    } else {
        if (info.st_mode & S_IFDIR) {

        } else {
            remove("/opt/USDot");
            isCreatedDir = true;
        }
    }
    if (isCreatedDir) {
        if (-1 == mkdir("/opt/USDot", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            PLEXUS_LOG(PLEXUS_ERROR, "create /opt/USDot directory fail");
        }        
    }
}

OCR_USDot::~OCR_USDot()
{
    if (m_enable)
    {
        /* Clean up nicely */

        PLEXUS_LOG(PLEXUS_INFO, "Stopping deepstream");
        gst_element_set_state (pipeline, GST_STATE_NULL);

        PLEXUS_LOG(PLEXUS_INFO, "Joining threads");

        g_main_loop_quit(m_gstLoop);
        m_stop = true;

        if (m_runThread.joinable())
        {
            m_runThread.join();
        }

        if (m_outputThread.joinable())
        {
            m_outputThread.join();
        }

        PLEXUS_LOG(PLEXUS_INFO, "Joined threads");

        /* Destroy context for Object Encoding */
        nvds_obj_enc_destroy_context(obj_ctx_handle);

        PLEXUS_LOG(PLEXUS_INFO, "Returned, stopping playback");
        gst_element_set_state(pipeline, GST_STATE_NULL);
        PLEXUS_LOG(PLEXUS_INFO, "Deleting pipeline");
        gst_object_unref (GST_OBJECT (pipeline));
        g_source_remove (bus_watch_id);
        g_main_loop_unref (m_gstLoop);

        PLEXUS_LOG(PLEXUS_INFO, "Deepstream stopped");
        m_enable = false;
    }
}

bool OCR_USDot::InitRecognizer()
{
    if (m_enable) {
        PLEXUS_LOG(PLEXUS_ERROR, "ALPR pipeline is already created.");
        return false;
    }

    readConfig();

    /* Standard GStreamer initialization */
    gst_init (nullptr, nullptr);
    m_gstLoop = g_main_loop_new (NULL, FALSE);


    /* Create gstreamer elements */
    /* Create Pipeline element that will form a connection of other elements */
    pipeline = gst_pipeline_new ("alpr-appsrc-pipeline");

    if (nullptr == pipeline)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create pipeline");
        return false;
    }

    /* Source element for reading data from push function */
    source = gst_element_factory_make ("appsrc", "source");

    if (nullptr == source)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create source element");
        return false;
    }

    /* Create nvstreammux instance to form batches from one or more sources. */
    streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

    if (nullptr == streammux)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create muxer element");
        return false;
    }

    /* Use nvinfer to run inferencing on decoder's output,
     * behaviour of inferencing is set through config file */
    pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

    if (nullptr == pgie)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create primary inference element");
        return false;
    }

    if (m_enableDebugRender)
    {
        /* Use convertor to convert from NV12 to RGBA as required by nvosd */
        nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

        if (nullptr == nvvidconv)
        {
            PLEXUS_LOG(PLEXUS_ERROR, "Failed to create OSD converter element");
            return false;
        }

        /* Create OSD to draw on the converted RGBA buffer */
        nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

        if (nullptr == nvosd)
        {
            PLEXUS_LOG(PLEXUS_ERROR, "Failed to create OSD element");
            return false;
        }
    }

    queue_sink = gst_element_factory_make ("queue", "queue_sink");

    if (nullptr == queue_sink)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create queue sink element");
        return false;
    }

    nvvidconv_sink = gst_element_factory_make ("nvvideoconvert", "nvvidconv_sink");

    if (nullptr == nvvidconv_sink)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create sink converter element");
        return false;
    }

    /* Sink element for output of data */
    sink = gst_element_factory_make ("appsink", "outsink");

    if (nullptr == sink)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create sink element");
        return false;
    }

    /* Set input format */
    g_object_set (G_OBJECT (source),
                  "stream-type", GST_APP_STREAM_TYPE_STREAM,
                  "is_live", true,
                  "format", GST_FORMAT_TIME,
                  nullptr);

    g_object_set (G_OBJECT (streammux), "live-source", muxer_live_source, NULL);
    g_object_set (G_OBJECT (streammux), "batch-size", muxer_batch_size, NULL);
    g_object_set (G_OBJECT (streammux),
        "width", muxer_width, "height", muxer_height,
        "batched-push-timeout", muxer_batched_push_timeout, NULL);

    /* Set all the necessary properties of the nvinfer element,
     * the necessary ones are : */
    g_object_set (G_OBJECT (pgie), "config-file-path", PGIE_CONFIG_FILE, NULL);

#ifdef TRANSCODE_VIDEO

    /* Since the data format in the input file is elementary h264 stream,
     * we need a h264parser */
    h264parser = gst_element_factory_make ("h264parse", "h264-parser");

    if (nullptr == h264parser)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create source h264 parser element");
        return false;
    }

    /* Use nvdec_h264 for hardware accelerated decode on GPU */
    decoder = gst_element_factory_make ("nvv4l2decoder", "nvv4l2-decoder");

    if (nullptr == decoder)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create decoder element");
        return false;
    }

    /* Use nvdec_h264 for hardware accelerated encode on GPU */
    encoder = gst_element_factory_make ("nvv4l2h264enc", "nvv4l2-encoder");

    if (nullptr == encoder)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create encoder element");
        return false;
    }

    h264parser_sink = gst_element_factory_make ("h264parse", "h264-parser-sink");

    if (nullptr == h264parser_sink)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create sink h264 parser element");
        return false;
    }

    g_object_set (G_OBJECT (encoder), "bitrate", 5000000, NULL);

    g_object_set (G_OBJECT (h264parser_sink), "config-interval", -1, NULL);

#else

    nvvidconv_src = gst_element_factory_make ("nvvideoconvert", "nvvidconv_src");

    if (nullptr == nvvidconv_src)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create source converter element");
        return false;
    }

    caps_filter_src = gst_element_factory_make ("capsfilter", "capsfilter_src");

    if (nullptr == caps_filter_src)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create source filter element");
        return false;
    }

    caps_filter_sink = gst_element_factory_make ("capsfilter", "capsfilter_sink");

    if (nullptr == caps_filter_sink)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to create sink filter element");
        return false;
    }

    GstCaps *caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "NV12", NULL);
    GstCapsFeatures *feature = gst_caps_features_new ("memory:NVMM", NULL);
    gst_caps_set_features (caps, 0, feature);
    g_object_set (G_OBJECT (caps_filter_src), "caps", caps, NULL);
    gst_caps_unref(caps);

    GstCaps *sinkcaps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "I420", NULL);
    g_object_set (G_OBJECT (caps_filter_sink), "caps", sinkcaps, NULL);
    gst_caps_unref(sinkcaps);

#endif

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, m_gstLoop);
    gst_object_unref (bus);

    /* Set up the pipeline */
    /* we add all elements into the pipeline */
    if (m_enableDebugRender)
    {
        gst_bin_add_many (GST_BIN (pipeline),
            source, streammux, pgie, nvvidconv, nvosd, queue_sink, nvvidconv_sink, sink, NULL);
    }
    else
    {
        gst_bin_add_many (GST_BIN (pipeline),
            source, streammux, pgie, queue_sink, nvvidconv_sink, sink, NULL);
    }

#ifdef TRANSCODE_VIDEO
    gst_bin_add_many (GST_BIN (pipeline), h264parser, decoder, encoder, h264parser_sink, NULL);
#else
    gst_bin_add_many (GST_BIN (pipeline), nvvidconv_src, caps_filter_src, caps_filter_sink, NULL);
#endif

    GstPad *pgie_src_pad;
    pgie_src_pad = gst_element_get_static_pad(pgie, "src");
    /*Creat Context for Object Encoding */
    obj_ctx_handle = nvds_obj_enc_create_context();
    if (nullptr == obj_ctx_handle)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "Unable to create context");
        return false;
    }
    if (nullptr == pgie_src_pad)
        PLEXUS_LOG(PLEXUS_ERROR, "Unable to get src pad");
    else
    {
        gst_pad_add_probe(pgie_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
                        pgie_src_pad_buffer_probe, (gpointer)obj_ctx_handle, NULL);
        gst_object_unref(pgie_src_pad);
    }

    GstPad *sinkpad, *srcpad;
    gchar pad_name_sink[16] = "sink_0";
    gchar pad_name_src[16] = "src";

    sinkpad = gst_element_get_request_pad (streammux, pad_name_sink);
    if (!sinkpad) {
      PLEXUS_LOG(PLEXUS_ERROR, "Request sink pad failed. Exiting");
      return false;
    }

#ifdef TRANSCODE_VIDEO
    GstElement *padSrcElement = decoder;
#else
    GstElement *padSrcElement = caps_filter_src;
#endif

    srcpad = gst_element_get_static_pad (padSrcElement, pad_name_src);
    if (!srcpad) {
      PLEXUS_LOG(PLEXUS_ERROR, "Request src pad failed. Exiting");
      return false;
    }

    if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
        PLEXUS_LOG(PLEXUS_ERROR, "Failed to link pads. Exiting");
        return false;
    }

    gst_object_unref (sinkpad);
    gst_object_unref (srcpad);

#ifdef TRANSCODE_VIDEO
    bool sourceLinkRes = gst_element_link_many(source, h264parser, decoder, NULL);
#else
    bool sourceLinkRes = gst_element_link_many(source, nvvidconv_src, caps_filter_src, NULL);
#endif

    if (!sourceLinkRes) {
      PLEXUS_LOG(PLEXUS_ERROR, "Source elements could not be linked. Exiting");
      return false;
    }

#ifdef TRANSCODE_VIDEO
    bool sinkLinkRes = gst_element_link_many (streammux, pgie, nvvidconv, nvosd, queue_sink, nvvidconv_sink, encoder, h264parser_sink, sink, NULL);
#else
    bool sinkLinkRes = false;

    if (m_enableDebugRender)
    {
        sinkLinkRes = gst_element_link_many (streammux, pgie, nvvidconv, nvosd, queue_sink, nvvidconv_sink, caps_filter_sink, sink, NULL);
    }
    else
    {
        sinkLinkRes = gst_element_link_many (streammux, pgie, queue_sink, nvvidconv_sink, caps_filter_sink, sink, NULL);
    }
#endif

    if (!sinkLinkRes) {
      PLEXUS_LOG(PLEXUS_ERROR, "Sink elements could not be linked. Exiting");
      return false;
    }

    /* Lets add probe to get informed of the meta data generated, we add probe to
     * the sink pad of the osd element, since by that time, the buffer would have
     * had got all the metadata. */
    osd_sink_pad = gst_element_get_static_pad (nvosd, "sink");
    if (!osd_sink_pad)
      PLEXUS_LOG(PLEXUS_INFO, "Unable to get sink pad\n");
    else
      gst_pad_add_probe (osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
          osd_sink_pad_buffer_probe, NULL, NULL);
    gst_object_unref (osd_sink_pad);

    /* Set the pipeline to "playing" state */
    PLEXUS_LOG(PLEXUS_INFO, "Now playing");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Wait till pipeline encounters an error or EOS */
    PLEXUS_LOG(PLEXUS_INFO, "Running...");

    m_stop = false;
    m_runThread = std::thread(g_main_loop_run, m_gstLoop);
    m_outputThread = std::thread(&OCR_USDot::GetOutputThread, this);

    m_start = std::chrono::steady_clock::now();
    m_enable = true;
    return true;
}

void OCR_USDot::DoTextDetect(cv::Mat &inputImg, std::vector<cv::Mat> &originalImages, std::vector<cv::Point> &locations,
                                const JDL::DateTime &timestamp, bool tryInverted, bool ignoreResult)
{
    if (!m_enable) {
        PLEXUS_LOG(PLEXUS_ERROR, "Filter is not inited.");
        return;
    }
    int srcFrameHeight = inputImg.rows;
    int srcFrameWidth = inputImg.cols;
    cv::Mat imageYUVI420;
    cv::cvtColor(inputImg, imageYUVI420, cv::COLOR_BGR2YUV_I420);
    buffInputImage.mat = inputImg.clone();
    buffInputImage.frameWidth = srcFrameWidth;
    buffInputImage.frameHeight = srcFrameHeight;
    buffInputImage.ignoreResult = ignoreResult;
    memcpy(&buffInputImage.timestamp, &timestamp, sizeof(JDL::DateTime));

    GstBuffer *pBuf = gst_buffer_new_and_alloc(imageYUVI420.total());
    gsize bytesCopied = gst_buffer_fill(pBuf, 0, imageYUVI420.data, imageYUVI420.total());

    auto finish = std::chrono::steady_clock::now();
    GST_BUFFER_PTS(pBuf) = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - m_start).count();

#ifdef TRANSCODE_VIDEO
    std::string inputMediaType = "video/x-h264";
#else
    std::string inputMediaType = "video/x-raw";
#endif

    GstCaps *srcCaps = gst_caps_new_simple(inputMediaType.c_str(),
                                            "format", G_TYPE_STRING, "I420",
                                            "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                            "width", G_TYPE_INT, srcFrameWidth,
                                            "height", G_TYPE_INT, srcFrameHeight,
                                            "framerate", GST_TYPE_FRACTION, 0, 1,
                                            nullptr);

    GstSample *pSample = gst_sample_new(pBuf, srcCaps, nullptr, nullptr);

    gst_app_src_push_sample(GST_APP_SRC(source), pSample);

    gst_sample_unref(pSample);
    gst_caps_unref(srcCaps);

    gst_buffer_unref(pBuf);
}



void OCR_USDot::GetOutputThread()
{
    gint output_threshold = 0;
    gint lpr_word_limit = 0;
    guint lpr_word_count = 7;
    float output_threshold_float = 0.0;
    bool mustContainPlate = false;
    while (!m_stop)
    {
        GstSample *pSample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        if (nullptr == pSample)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            PLEXUS_LOG(PLEXUS_ERROR, "Failed to get sample");
            continue;
        }
        GstBuffer* pBuffer = gst_sample_get_buffer(pSample);
        GstMapInfo map;
        gst_buffer_map(pBuffer, &map, GST_MAP_READ);

        while (!OCR_USDot_Result.empty()) {
            USDotResult result = OCR_USDot_Result.front();
            PLEXUS_LOG(PLEXUS_DEBUG, "[Detect]id=%u, text=%s, conf=%d", 
                result.id, result.text.c_str(), result.confidence);

            cv::Mat resultImg = buffInputImage.mat;;

            if (!buffInputImage.ignoreResult) {
                std::string vehicleType = "Unrecognized";
                std::string region;
                std::string model = "Unrecognized";
                std::string color = "Unrecognized";
                std::string make = "Unrecognized";
                PLEXUS_LOG(PLEXUS_DEBUG, "call HandleNewResult to send event (text:%s, conf:%d", 
                            result.text.c_str(), result.confidence);
                HandleNewResult(resultImg, buffInputImage.timestamp,result.text, result.confidence,
                                vehicleType, 0,
                                region, 0,
                                make, model, 0,
                                color, 0);
            }

            OCR_USDot_Result.pop_front();
        }

        gst_buffer_unmap(pBuffer, &map);
        gst_sample_unref(pSample);
    }
}

void OCR_USDot::readConfig()
{
    char name[USDOT_SIZE];
    char value[USDOT_SIZE];

    FILE *fp = fopen(CONFIG_PATH, "r");
    if (fp == NULL)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "open %s fail", CONFIG_PATH);
        return;
    }
    else
    {
        while (!feof(fp))
        {
            memset(name, 0, USDOT_SIZE);
            memset(value, 0, USDOT_SIZE);

            /*Read Data*/
            fscanf(fp, "%s = %s\n", name, value);

            if (!strcmp(name, "muxer_width"))
            {
                muxer_width = atoi(value);
            }
            else if (!strcmp(name, "muxer_height"))
            {
                muxer_height = atoi(value);
            }
            else if (!strcmp(name, "muxer_live_source"))
            {
                muxer_live_source = atoi(value);
            }
            else if (!strcmp(name, "muxer_batch_size"))
            {
                muxer_batch_size = atoi(value);
            }
            else if (!strcmp(name, "muxer_batched_push_timeout"))
            {
                muxer_batched_push_timeout = atoi(value);
            }
            else if (!strcmp(name, "muxer_nvbuf_memory_type"))
            {
                muxer_nvbuf_memory_type = atoi(value);
            }
            else if (!strcmp(name, "open_rec"))
            {
                open_rec = atoi(value);
            }
            else if (!strcmp(name, "rec_min_width"))
            {
                rec_min_width = atoi(value);
            }
            else if (!strcmp(name, "rec_min_height"))
            {
                rec_min_height = atoi(value);
            }
        }
    }
    fclose(fp);
}

struct usdot_string
{
    char *ptr;
    size_t len;
};

void init_string(struct usdot_string *s)
{
    s->len = 0;
    s->ptr = (char *)malloc(s->len + 1);
    if (s->ptr == NULL)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct usdot_string *s)
{
    size_t new_len = s->len + size * nmemb;
    s->ptr = (char*)realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL)
    {
        PLEXUS_LOG(PLEXUS_ERROR, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size * nmemb;
}

/* pgie_src_pad_buffer_probe will extract metadata received on pgie src pad
 * and update params for drawing rectangle, object information etc. We also
 * iterate through the object list and encode the cropped objects as jpeg
 * images and attach it as user meta to the respective objects.*/
GstPadProbeReturn
OCR_USDot::pgie_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info,
                          gpointer ctx)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    GstMapInfo inmap = GST_MAP_INFO_INIT;
    if (!gst_buffer_map(buf, &inmap, GST_MAP_READ))
    {
        PLEXUS_LOG(PLEXUS_ERROR, "input buffer mapinfo failed");
        return GST_PAD_PROBE_DROP;
    }
    NvBufSurface *ip_surf = (NvBufSurface *)inmap.data;
    gst_buffer_unmap(buf, &inmap);

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next)
    {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        if (frame_meta->obj_meta_list == NULL)
        {
            continue;
        }

        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next)
        {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);

            /* Conditions that user needs to set to encode the detected objects of
            * interest. Here, by default all the detected objects are encoded.
            * For demonstration, we will encode the first object in the frame */
            if (obj_meta->class_id == 0 && obj_meta->detector_bbox_info.org_bbox_coords.width >= rec_min_width && obj_meta->detector_bbox_info.org_bbox_coords.height >= rec_min_height)
            {
                NvDsObjEncUsrArgs userData = {0};
                /* To be set by user */
                userData.saveImg = save_img;
                userData.attachUsrMeta = attach_user_meta;
                /* Quality */
                userData.quality = 100;
                // /* Set Image WHTL */
                obj_meta->rect_params.width = obj_meta->detector_bbox_info.org_bbox_coords.width;
                obj_meta->rect_params.height = obj_meta->detector_bbox_info.org_bbox_coords.height;
                obj_meta->rect_params.top = obj_meta->detector_bbox_info.org_bbox_coords.top;
                obj_meta->rect_params.left = obj_meta->detector_bbox_info.org_bbox_coords.left;
                /* Set image name */
                snprintf(userData.fileNameImg, sizeof(userData.fileNameImg), "/opt/USDot/ocr.jpg");
                /*Main Function Call */
                if (open_rec)
                {
                    nvds_obj_enc_process((NvDsObjEncCtxHandle)ctx, &userData, ip_surf, obj_meta, frame_meta);
                    PLEXUS_LOG(PLEXUS_DEBUG, "Car is detected and snapshot on /opt/USDot/ocr.jpg");
                    CURL *curl;
                    CURLcode res;
                    curl = curl_easy_init(); // 初始化
                    if (curl)
                    {
                        struct usdot_string s;
                        init_string(&s);

                        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/inference");
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
                        res = curl_easy_perform(curl); // 執行

                        if (strcmp(s.ptr, "\"\"") != 0 && strcmp(s.ptr, "") != 0)
                        {
                            gchar original_response[s.len];
                            gchar output_response[s.len];
                            memset(original_response, 0, s.len);
                            memset(output_response, 0, s.len);
                            strcpy(original_response, &s.ptr[1]);
                            strncat(output_response, original_response, s.len - 2);
                            g_print("%s, %f\n", output_response, obj_meta->confidence);

                            USDotResult result;
                            result.id = obj_meta->object_id;
                            result.text = std::string(output_response);
                            result.confidence = obj_meta->confidence * 100;
                            OCR_USDot_Result.push_back(result);

                        } else {
                            PLEXUS_LOG(PLEXUS_DEBUG, "There are no USDot texts.");
                        }
                        free(s.ptr);

                        if (res != 0)
                        {
                            curl_easy_cleanup(curl);
                        }
                    }
                }
            }
        }
    }
    nvds_obj_enc_finish((NvDsObjEncCtxHandle)ctx);
    return GST_PAD_PROBE_OK;
}


/* This is the buffer probe function that we have registered on the sink pad
 * of the OSD element. All the infer elements in the pipeline shall attach
 * their metadata to the GstBuffer, here we will iterate & process the metadata
 * forex: class ids to strings, counting of class_id objects etc. */
GstPadProbeReturn
OCR_USDot::osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info,
                          gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    guint num_rects = 0;
    NvDsObjectMeta *obj_meta = NULL;
    guint vehicle_count = 0;
    guint person_count = 0;
    NvDsMetaList *l_frame = NULL;
    NvDsMetaList *l_obj = NULL;
    NvDsDisplayMeta *display_meta = NULL;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    int current_device = -1;
    cudaGetDevice(&current_device);
    struct cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, current_device);

    return GST_PAD_PROBE_OK;
}

gboolean
OCR_USDot::bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:
            {
                gchar *debug;
                GError *error;
                gst_message_parse_error(msg, &error, &debug);
                PLEXUS_LOG(PLEXUS_ERROR, "ERROR from element %s: %s\n",
                            GST_OBJECT_NAME(msg->src), error->message);
                if (debug)
                    g_printerr("Error details: %s\n", debug);
                g_free(debug);
                g_error_free(error);
                g_main_loop_quit(loop);
                break;
            }
        default:
            break;
    }
    return TRUE;
}

