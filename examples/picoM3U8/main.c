#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This one is purely for the purpose of debugging (using DebugPrint functions of picoM3U8)
#define PICO_M3U8_LOG(...) do { \
    printf(__VA_ARGS__); \
    printf("\n"); \
 } while(0)

#define PICO_IMPLEMENTATION
#include "pico/picoM3U8.h"

const char *SAMPLE_PLAYLISTS[] = {
    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-TARGETDURATION:10\n"
    "#EXT-X-VERSION:3\n"
    "#EXTINF:9.009,\n"
    "http://media.example.com/first.ts\n"
    "#EXTINF:9.009,\n"
    "http://media.example.com/second.ts\n"
    "#EXTINF:3.003,\n"
    "http://media.example.com/third.ts\n"
    "#EXT-X-ENDLIST\n",
    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-VERSION:3\n"
    "#EXT-X-TARGETDURATION:8\n"
    "#EXT-X-MEDIA-SEQUENCE:2680\n"
    "\n"
    "#EXTINF:7.975,\n"
    "https://priv.example.com/fileSequence2680.ts\n"
    "#EXTINF:7.941,\n"
    "https://priv.example.com/fileSequence2681.ts\n"
    "#EXTINF:7.975,\n"
    "https://priv.example.com/fileSequence2682.ts\n",
    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-VERSION:3\n"
    "#EXT-X-MEDIA-SEQUENCE:7794\n"
    "#EXT-X-TARGETDURATION:15\n"
    "\n"
    "#EXT-X-KEY:METHOD=AES-128,URI=\"https://priv.example.com/key.php?r=52\"\n"
    "\n"
    "#EXTINF:2.833,\n"
    "http://media.example.com/fileSequence52-A.ts\n"
    "#EXTINF:15.0,\n"
    "http://media.example.com/fileSequence52-B.ts\n"
    "#EXTINF:13.333,\n"
    "http://media.example.com/fileSequence52-C.ts\n"
    "\n"
    "#EXT-X-KEY:METHOD=AES-128,URI=\"https://priv.example.com/key.php?r=53\"\n"
    "\n"
    "#EXTINF:15.0,\n"
    "http://media.example.com/fileSequence53-A.ts\n",
    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=1280000,AVERAGE-BANDWIDTH=1000000\n"
    "http://example.com/low.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=2560000,AVERAGE-BANDWIDTH=2000000\n"
    "http://example.com/mid.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=7680000,AVERAGE-BANDWIDTH=6000000\n"
    "http://example.com/hi.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=65000,CODECS=\"mp4a.40.5\"\n"
    "http://example.com/audio-only.m3u8\n",
    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-TARGETDURATION:7\n"
    "#EXT-X-VERSION:3\n"
    "#EXT-X-MEDIA-SEQUENCE:354770\n"
    "#EXT-X-PROGRAM-DATE-TIME:2025-10-26T23:14:49.505Z\n"
    "#EXTINF:6.520,\n"
    "2025/10/26/23/14/49-06520.ts\n"
    "#EXTINF:5.120,\n"
    "2025/10/26/23/14/56-05120.ts\n"
    "#EXTINF:5.120,\n"
    "2025/10/26/23/15/01-05120.ts\n"
    "#EXTINF:5.120,\n"
    "2025/10/26/23/15/06-05120.ts\n",

    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=1280000\n"
    "low/audio-video.m3u8\n"
    "#EXT-X-I-FRAME-STREAM-INF:BANDWIDTH=86000,URI=\"low/iframe.m3u8\"\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=2560000\n"
    "mid/audio-video.m3u8\n"
    "#EXT-X-I-FRAME-STREAM-INF:BANDWIDTH=150000,URI=\"mid/iframe.m3u8\"\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=7680000\n"
    "hi/audio-video.m3u8\n"
    "#EXT-X-I-FRAME-STREAM-INF:BANDWIDTH=550000,URI=\"hi/iframe.m3u8\"\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=65000,CODECS=\"mp4a.40.5\"\n"
    "audio-only.m3u8\n",
    // ----------------------------
    "#EXTM3U\n"
    "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aac\",NAME=\"English\",DEFAULT=YES,AUTOSELECT=YES,LANGUAGE=\"en\",URI=\"main/english-audio.m3u8\"\n"
    "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aac\",NAME=\"Deutsch\",DEFAULT=NO,AUTOSELECT=YES,LANGUAGE=\"de\",URI=\"main/german-audio.m3u8\"\n"
    "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aac\",NAME=\"Commentary\",DEFAULT=NO,AUTOSELECT=NO,LANGUAGE=\"en\",URI=\"commentary/audio-only.m3u8\"\n"
    "#EXT-X-SESSION-DATA:DATA-ID=\"com.example.lyrics\",URI=\"lyrics.json\"\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=1280000,CODECS=\"...\",AUDIO=\"aac\"\n"
    "low/video-only.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=2560000,CODECS=\"...\",AUDIO=\"aac\"\n"
    "mid/video-only.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=7680000,CODECS=\"...\",AUDIO=\"aac\"\n"
    "hi/video-only.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=65000,CODECS=\"mp4a.40.5\",AUDIO=\"aac\"\n"
    "main/english-audio.m3u8\n",
    // ----------------------------
};

int main(void)
{
    printf("Hello, Pico!\n");

    size_t sampleCount = sizeof(SAMPLE_PLAYLISTS) / sizeof(SAMPLE_PLAYLISTS[0]);
    for (size_t i = 0; i < sampleCount; ++i) {
        const char *playlistData = SAMPLE_PLAYLISTS[i];
        uint32_t dataLength = (uint32_t)strlen(playlistData);
        printf("Parsing Playlist %zu...\n", i + 1);
        picoM3U8Playlist playlist;
        picoM3U8Result result = picoM3U8PlaylistParse(playlistData, dataLength, &playlist);
        if (result != PICO_M3U8_RESULT_SUCCESS) {
            const char *resultStr = picoM3U8ResultToString(result);
            printf("  Failed to parse playlist: %s\n", resultStr);
        } else {
            printf("  Successfully parsed playlist of type: %s\n", picoM3U8PlaylistTypeToString(playlist->type));
            picoM3U8PlaylistDebugPrint(playlist);
            picoM3U8PlaylistDestroy(playlist);
        }
        printf("------------------------------\n");
    }

    printf("Goodbye, Pico!\n");

    return 0;
}
