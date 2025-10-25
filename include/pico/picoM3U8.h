/*
MIT License

Copyright (c) 2025 Jaysmito Mukherjee (jaysmito101@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// NOTE: docs taken directly from the RFC: https://datatracker.ietf.org/doc/html/rfc8216

#ifndef PICO_M3U8_H
#define PICO_M3U8_H


#include <stdbool.h>
#include <stdint.h>

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE   free
#endif

#ifndef PICO_M3U8_MAX_URI_LENGTH
#define PICO_M3U8_MAX_URI_LENGTH 2048
#endif

#ifndef PICO_M3U8_MAX_DATATIME_STRING_LENGTH
#define PICO_M3U8_MAX_DATATIME_STRING_LENGTH 64
#endif

#ifndef PICO_M3U8_MAX_STRING_ITEM_LENGTH
#define PICO_M3U8_MAX_STRING_ITEM_LENGTH 256
#endif

#ifndef PICO_M3U8_MAX_MEDIA_CHARACTERISTICS_LENGTH
#define PICO_M3U8_MAX_MEDIA_CHARACTERISTICS_LENGTH 256
#endif

#ifndef PICO_M3U8_MAX_MEDIA_CHARACTERISTICS_COUNT
#define PICO_M3U8_MAX_MEDIA_CHARACTERISTICS_COUNT 8
#endif

#ifndef PICO_M3U8_MAX_MEDIA_RENDITIONS
#define PICO_M3U8_MAX_MEDIA_RENDITIONS 8
#endif

#ifndef PICO_M3U8_MAX_VARIANT_STREAMS
#define PICO_M3U8_MAX_VARIANT_STREAMS 16
#endif

#ifndef PICO_M3U8_MAX_STREAM_CODECS
#define PICO_M3U8_MAX_STREAM_CODECS 4
#endif

#ifndef PICO_M3U8_MAX_SESSION_DATA
#define PICO_M3U8_MAX_SESSION_DATA 2
#endif

#ifndef PICO_M3U8_MAX_SESSION_KEYS
#define PICO_M3U8_MAX_SESSION_KEYS 2
#endif

#ifndef PICO_M3U8_MAX_MEDIA_SEGMENTS
#define PICO_M3U8_MAX_MEDIA_SEGMENTS 256
#endif

typedef enum {
    PICO_M3U8_PLAYLIST_TYPE_MASTER = 0,
    PICO_M3U8_PLAYLIST_TYPE_MEDIA,
    PICO_M3U8_PLAYLIST_TYPE_COUNT
} picoM3U8PlaylistType;

typedef enum {
    PICO_M3U8_MEDIA_TYPE_ATTRIBUTE_UNKNOWN = 0,
    PICO_M3U8_MEDIA_TYPE_ATTRIBUTE_VIDEO,
    PICO_M3U8_MEDIA_TYPE_ATTRIBUTE_AUDIO,
    PICO_M3U8_MEDIA_TYPE_ATTRIBUTE_SUBTITLES,
    PICO_M3U8_MEDIA_TYPE_ATTRIBUTE_CLOSED_CAPTIONS,
    PICO_M3U8_MEDIA_TYPE_ATTRIBUTE_COUNT
} picoM3U8MediaType;

typedef enum {
    PICO_M3U8_INSTREAM_ID_CC1 = 0,
    PICO_M3U8_INSTREAM_ID_CC2,
    PICO_M3U8_INSTREAM_ID_CC3,
    PICO_M3U8_INSTREAM_ID_CC4,
    PICO_M3U8_INSTREAM_ID_SERVICE,
    PICO_M3U8_INSTREAM_ID_COUNT
} picoM3U8InstreamIdType;

typedef enum {
    PICO_M3U8_HDCP_LEVEL_NONE = 0,
    PICO_M3U8_HDCP_LEVEL_TYPE0,
    PICO_M3U8_HDCP_LEVEL_COUNT
} picoM3U8HDCPLevel;

typedef enum {
    PICO_M3U8_KEY_METHOD_NONE = 0,
    PICO_M3U8_KEY_METHOD_AES_128,
    PICO_M3U8_KEY_METHOD_SAMPLE_AES,
    PICO_M3U8_KEY_METHOD_COUNT
} picoM3U8KeyMethod;

typedef enum {
    PICO_M3U8_MEDIA_PLAYLIST_TYPE_VOD = 0,
    PICO_M3U8_MEDIA_PLAYLIST_TYPE_EVENT,
    PICO_M3U8_MEDIA_PLAYLIST_TYPE_COUNT
} picoM3U8MediaPlaylistType;

typedef union {
    picoM3U8InstreamIdType type;
    struct {
        picoM3U8InstreamIdType type;
        uint8_t n; // n is an integer between 1 and 63
    } service;
} picoM3U8InstreamId;

typedef struct {
    // The value is an enumerated-string that specifies the encryption
    // method.  This attribute is REQUIRED.
    // The methods defined are: NONE, AES-128, and SAMPLE-AES.
    // An encryption method of NONE means that Media Segments are not
    // encrypted.  If the encryption method is NONE, other attributes
    // MUST NOT be present.
    // An encryption method of AES-128 signals that Media Segments are
    // completely encrypted using the Advanced Encryption Standard (AES)
    // [AES_128] with a 128-bit key, Cipher Block Chaining (CBC), and
    // Public-Key Cryptography Standards #7 (PKCS7) padding [RFC5652].
    // CBC is restarted on each segment boundary, using either the
    // Initialization Vector (IV) attribute value or the Media Sequence
    // Number as the IV;
    // An encryption method of SAMPLE-AES means that the Media Segments
    // contain media samples, such as audio or video, that are encrypted
    // using the Advanced Encryption Standard [AES_128].  How these media
    // streams are encrypted and encapsulated in a segment depends on the
    // media encoding and the media format of the segment.  fMP4 Media
    // Segments are encrypted using the 'cbcs' scheme of Common
    // Encryption [COMMON_ENC].  Encryption of other Media Segment
    // formats containing H.264 [H_264], AAC [ISO_14496], AC-3 [AC_3],
    // and Enhanced AC-3 [AC_3] media streams is described in the HTTP
    // Live Streaming (HLS) Sample Encryption specification [SampleEnc].
    // The IV attribute MAY be present
    picoM3U8KeyMethod method;

    // The value is a quoted-string containing a URI that specifies how
    // to obtain the key.  This attribute is REQUIRED unless the METHOD
    // is NONE.
    char uri[PICO_M3U8_MAX_URI_LENGTH];

    // The value is a hexadecimal-sequence that specifies a 128-bit
    // unsigned integer Initialization Vector to be used with the key.
    // Use of the IV attribute REQUIRES a compatibility version number of
    // 2 or greater.
    char iv[128 / 8];

    // The value is a quoted-string that specifies how the key is
    // represented in the resource identified by the URI; 
    // This attribute is OPTIONAL; its absence
    // indicates an implicit value of "identity".  Use of the KEYFORMAT
    // attribute REQUIRES a compatibility version number of 5 or greater.
    char keyFormat[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is a quoted-string containing one or more positive
    // integers separated by the "/" character (for example, "1", "1/2",
    // or "1/2/5").  If more than one version of a particular KEYFORMAT
    // is defined, this attribute can be used to indicate which
    // version(s) this instance complies with.  This attribute is
    // OPTIONAL; if it is not present, its value is considered to be "1".
    // Use of the KEYFORMATVERSIONS attribute REQUIRES a compatibility
    // version number of 5 or greater.
    char keyFormatVersions[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

} picoM3U8KeyAttributes_t;
typedef picoM3U8KeyAttributes_t *picoM3U8KeyAttributes;

typedef struct {
    // The value of TIME-OFFSET is a signed-decimal-floating-point number
    // of seconds.  A positive number indicates a time offset from the
    // beginning of the Playlist.  A negative number indicates a negative
    // time offset from the end of the last Media Segment in the
    // Playlist.  This attribute is REQUIRED.
    // The absolute value of TIME-OFFSET SHOULD NOT be larger than the
    // Playlist duration.  If the absolute value of TIME-OFFSET exceeds
    // the duration of the Playlist, it indicates either the end of the
    // Playlist (if positive) or the beginning of the Playlist (if
    // negative).
    // If the Playlist does not contain the EXT-X-ENDLIST tag, the TIME-
    // OFFSET SHOULD NOT be within three target durations of the end of
    // the Playlist file.
    float timeOffset;

    // The value is an enumerated-string; valid strings are YES and NO.
    // If the value is YES, clients SHOULD start playback at the Media
    // Segment containing the TIME-OFFSET, but SHOULD NOT render media
    // samples in that segment whose presentation times are prior to the
    // TIME-OFFSET.  If the value is NO, clients SHOULD attempt to render
    // every media sample in that segment.  This attribute is OPTIONAL.
    // If it is missing, its value should be treated as NO.
    bool precise;
} picoM3U8StartAttributes_t;
typedef picoM3U8StartAttributes_t *picoM3U8StartAttributes;

typedef struct {
    uint32_t length;
    uint32_t offset;
    bool hasOffset;
} picoM3U8ByteRange;

typedef struct {
    // The URI attribute is a quoted-string containing a URI that
    // specifies how to obtain the Media Initialization Section.
    char uri[PICO_M3U8_MAX_URI_LENGTH];

    // The value is a quoted-string specifying a byte range into the
    // resource identified by the URI attribute.  This range SHOULD
    // contain only the Media Initialization Section. This attribute is
    // OPTIONAL; if it is not present, the byte range is the entire
    // resource indicated by the URI.
    picoM3U8ByteRange byteRange;
    bool hasByteRange;
} picoM3U8Map;

typedef struct {
    char data[PICO_M3U8_MAX_DATATIME_STRING_LENGTH];
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t millisecond;
    int8_t timezoneOffset;
} picoM3U8DateTime_t;
typedef picoM3U8DateTime_t *picoM3U8DateTime;

typedef struct {
    // A quoted-string that uniquely identifies a Date Range in the
    // Playlist.  This attribute is REQUIRED.
    char id[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // A client-defined quoted-string that specifies some set of
    // attributes and their associated value semantics.  All Date Ranges
    // with the same CLASS attribute value MUST adhere to these
    // semantics.  This attribute is OPTIONAL.
    char class[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // A quoted-string containing the ISO-8601 date at which the Date
    // Range begins.  This attribute is REQUIRED.
    picoM3U8DateTime_t startDate;

    // A quoted-string containing the ISO-8601 date at which the Date
    // Range ends.  It MUST be equal to or later than the value of the
    // START-DATE attribute.  This attribute is OPTIONAL.
    picoM3U8DateTime_t endDate;

    // The duration of the Date Range expressed as a decimal-floating-
    // point number of seconds.  It MUST NOT be negative.  A single
    // instant in time (e.g., crossing a finish line) SHOULD be
    // represented with a duration of 0.  This attribute is OPTIONAL.
    float duration;

    // The expected duration of the Date Range expressed as a decimal-
    // floating-point number of seconds.  It MUST NOT be negative.  This
    // attribute SHOULD be used to indicate the expected duration of a
    // Date Range whose actual duration is not yet known.  It is
    // OPTIONAL.
    float plannedDuration;

    // NOTE: This library doesnt parse the X-* client attributes

    // An enumerated-string whose value MUST be YES.  This attribute
    // indicates that the end of the range containing it is equal to the
    // START-DATE of its Following Range.  The Following Range is the
    // Date Range of the same CLASS that has the earliest START-DATE
    // after the START-DATE of the range in question.  This attribute is
    // OPTIONAL.
    bool endOnNext;
} picoM3U8DateRange_t;
typedef picoM3U8DateRange_t *picoM3U8DateRange;

typedef struct {
    // The EXTINF tag specifies the duration of a Media Segment.  It applies
    // only to the next Media Segment.  This tag is REQUIRED for each Media
    // Segment.
    // where duration is a decimal-floating-point or decimal-integer number
    // (as described in Section 4.2) that specifies the duration of the
    // Media Segment in seconds.  Durations SHOULD be decimal-floating-
    // point, with enough accuracy to avoid perceptible error when segment
    // durations are accumulated.  However, if the compatibility version
    // number is less than 3, durations MUST be integers.  Durations that
    // are reported as integers SHOULD be rounded to the nearest integer.
    // The remainder of the line following the comma is an optional human-
    // readable informative title of the Media Segment expressed as UTF-8
    // text.
    float duration;

    // The EXT-X-BYTERANGE tag indicates that a Media Segment is a sub-range
    // of the resource identified by its URI.  It applies only to the next
    // URI line that follows it in the Playlist.  Its format is:
    // #EXT-X-BYTERANGE:<n>[@<o>]
    // where n is a decimal-integer indicating the length of the sub-range
    // in bytes.  If present, o is a decimal-integer indicating the start of
    // the sub-range, as a byte offset from the beginning of the resource.
    // If o is not present, the sub-range begins at the next byte following
    // the sub-range of the previous Media Segment.
    // If o is not present, a previous Media Segment MUST appear in the
    // Playlist file and MUST be a sub-range of the same media resource, or
    // the Media Segment is undefined and the client MUST fail to parse the
    // Playlist.
    // A Media Segment without an EXT-X-BYTERANGE tag consists of the entire
    // resource identified by its URI.
    // Use of the EXT-X-BYTERANGE tag REQUIRES a compatibility version
    // number of 4 or greater.
    picoM3U8ByteRange byteRange;
    bool hasByteRange;

    // The EXT-X-DISCONTINUITY tag indicates a discontinuity between the
    // Media Segment that follows it and the one that preceded it.
    // The EXT-X-DISCONTINUITY tag MUST be present if there is a change in
    // any of the following characteristics:
    // o  file format
    // o  number, type, and identifiers of tracks
    // o  timestamp sequence
    // The EXT-X-DISCONTINUITY tag SHOULD be present if there is a change in
    // any of the following characteristics:
    // o  encoding parameters
    // o  encoding sequence
    bool discontinuity;

    
    // Media Segments MAY be encrypted.  The EXT-X-KEY tag specifies how to
    // decrypt them.  It applies to every Media Segment and to every Media
    // Initialization Section declared by an EXT-X-MAP tag that appears
    // between it and the next EXT-X-KEY tag in the Playlist file with the
    // same KEYFORMAT attribute (or the end of the Playlist file).  Two or
    // more EXT-X-KEY tags with different KEYFORMAT attributes MAY apply to
    // the same Media Segment if they ultimately produce the same decryption
    // key. 
    picoM3U8KeyAttributes keyAttributes;
    bool hasKeyAttributes;

    // The EXT-X-MAP tag specifies how to obtain the Media Initialization
    // Section (Section 3) required to parse the applicable Media Segments.
    // It applies to every Media Segment that appears after it in the
    // Playlist until the next EXT-X-MAP tag or until the end of the
    // Playlist.
    // An EXT-X-MAP tag SHOULD be supplied for Media Segments in Playlists
    // with the EXT-X-I-FRAMES-ONLY tag when the first Media Segment (i.e.,
    // I-frame) in the Playlist (or the first segment following an EXT-
    // X-DISCONTINUITY tag) does not immediately follow the Media
    // Initialization Section at the beginning of its resource.
    // Use of the EXT-X-MAP tag in a Media Playlist that contains the EXT-
    // X-I-FRAMES-ONLY tag REQUIRES a compatibility version number of 5 or
    // greater.  Use of the EXT-X-MAP tag in a Media Playlist that DOES NOT
    // contain the EXT-X-I-FRAMES-ONLY tag REQUIRES a compatibility version
    // number of 6 or greater.
    // If the Media Initialization Section declared by an EXT-X-MAP tag is
    // encrypted with a METHOD of AES-128, the IV attribute of the EXT-X-KEY
    // tag that applies to the EXT-X-MAP is REQUIRED.
    picoM3U8Map map;
    bool hasMap;

    // The EXT-X-PROGRAM-DATE-TIME tag associates the first sample of a
    // Media Segment with an absolute date and/or time.  It applies only to
    // the next Media Segment.
    picoM3U8DateTime programDateTime;

    // The EXT-X-DATERANGE tag associates a Date Range (i.e., a range of
    // time defined by a starting and ending date) with a set of attribute/
    // value pairs.
    picoM3U8DateRange dateRange;
} picoM3U8MediaSegment_t;
typedef picoM3U8MediaSegment_t *picoM3U8MediaSegment;

typedef struct {
    // The EXT-X-VERSION tag indicates the compatibility version of the
    // Playlist file, its associated media, and its server.  The version number
    // The EXT-X-VERSION tag applies to the entire Playlist file. 
    // It MUST appear in all Playlists containing tags or attributes that
    // are not compatible with protocol version 1 to support
    // interoperability with older clients.  Section 7 specifies the minimum
    // value of the compatibility version number for any given Playlist
    // file.
    // A Playlist file MUST NOT contain more than one EXT-X-VERSION tag.  If
    // a client encounters a Playlist with multiple EXT-X-VERSION tags, it
    // MUST fail to parse it.
    uint32_t version;


    
    // The EXT-X-INDEPENDENT-SEGMENTS tag indicates that all media samples
    // in a Media Segment can be decoded without information from other
    // segments.  It applies to every Media Segment in the Playlist. 
    // If the EXT-X-INDEPENDENT-SEGMENTS tag appears in a Master Playlist,
    // it applies to every Media Segment in every Media Playlist in the
    // Master Playlist.
    bool independentSegments;

    // The EXT-X-START tag indicates a preferred point at which to start
    // playing a Playlist.  By default, clients SHOULD start playback at
    // this point when beginning a playback session.  This tag is OPTIONAL.
    picoM3U8StartAttributes startAttributes;    
} picoM3U8CommonInfo_t;
typedef picoM3U8CommonInfo_t *picoM3U8CommonInfo;

typedef struct {
    // The value is an enumerated-string; valid strings are AUDIO, VIDEO,
    // SUBTITLES, and CLOSED-CAPTIONS.  This attribute is REQUIRED.
    // Typically, closed-caption [CEA608] media is carried in the video
    // stream.  Therefore, an EXT-X-MEDIA tag with TYPE of CLOSED-
    // CAPTIONS does not specify a Rendition; the closed-caption media is
    // present in the Media Segments of every video Rendition.
    picoM3U8MediaType type;

    // The value is a quoted-string containing a URI that identifies the
    // Media Playlist file.  This attribute is OPTIONAL.  If the TYPE is
    // CLOSED-CAPTIONS, the URI attribute MUST NOT be present.
    char uri[PICO_M3U8_MAX_URI_LENGTH];

    // The value is a quoted-string that specifies the group to which the
    // Rendition belongs.  This attribute is REQUIRED.
    char groupId[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is a quoted-string containing one of the standard Tags
    // for Identifying Languages [RFC5646], which identifies the primary
    // language used in the Rendition.  This attribute is OPTIONAL.
    char language[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is a quoted-string containing a language tag [RFC5646]
    // that identifies a language that is associated with the Rendition.
    // An associated language is often used in a different role than the
    // language specified by the LANGUAGE attribute (e.g., written versus
    // spoken or a fallback dialect).  This attribute is OPTIONAL.
    char assocLanguage[PICO_M3U8_MAX_STRING_ITEM_LENGTH];


    // The value is a quoted-string containing a human-readable
    // description of the Rendition.  If the LANGUAGE attribute is
    // present, then this description SHOULD be in that language.  This
    // attribute is REQUIRED.
    char name[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is an enumerated-string; valid strings are YES and NO.
    // If the value is YES, then the client SHOULD play this Rendition of
    // the content in the absence of information from the user indicating
    // a different choice.  This attribute is OPTIONAL.  Its absence
    // indicates an implicit value of NO.
    bool defaultValue;

    // The value is an enumerated-string; valid strings are YES and NO.
    // This attribute is OPTIONAL.  Its absence indicates an implicit
    // value of NO.  If the value is YES, then the client MAY choose to
    // play this Rendition in the absence of explicit user preference
    // because it matches the current playback environment, such as
    // chosen system language.
    // If the AUTOSELECT attribute is present, its value MUST be YES if
    // the value of the DEFAULT attribute is YES.
    bool autoSelect;


    // The value is an enumerated-string; valid strings are YES and NO.
    // This attribute is OPTIONAL.  Its absence indicates an implicit
    // value of NO.  The FORCED attribute MUST NOT be present unless the
    // TYPE is SUBTITLES.
    // A value of YES indicates that the Rendition contains content that
    // is considered essential to play.  When selecting a FORCED
    // Rendition, a client SHOULD choose the one that best matches the
    // current playback environment (e.g., language).
    // A value of NO indicates that the Rendition contains content that
    // is intended to be played in response to explicit user request.
    bool forced;

    // The value is a quoted-string that specifies a Rendition within the
    // segments in the Media Playlist.  This attribute is REQUIRED if the
    // TYPE attribute is CLOSED-CAPTIONS, in which case it MUST have one
    // of the values: "CC1", "CC2", "CC3", "CC4", or "SERVICEn" where n
    // MUST be an integer between 1 and 63 (e.g., "SERVICE3" or
    // "SERVICE42").
    // The values "CC1", "CC2", "CC3", and "CC4" identify a Line 21 Data
    // Services channel [CEA608].  The "SERVICE" values identify a
    // Digital Television Closed Captioning [CEA708] service block
    // number.
    // For all other TYPE values, the INSTREAM-ID MUST NOT be specified.
    picoM3U8InstreamId instreamId;
    
    // The value is a quoted-string containing one or more Uniform Type
    // Identifiers [UTI] separated by comma (,) characters.  This
    // attribute is OPTIONAL.  Each UTI indicates an individual
    // characteristic of the Rendition.
    // A SUBTITLES Rendition MAY include the following characteristics:
    // "public.accessibility.transcribes-spoken-dialog",
    // "public.accessibility.describes-music-and-sound", and
    // "public.easy-to-read" (which indicates that the subtitles have
    // been edited for ease of reading).
    // An AUDIO Rendition MAY include the following characteristic:
    // "public.accessibility.describes-video".
    // The CHARACTERISTICS attribute MAY include private UTIs.
    char characteristics[PICO_M3U8_MAX_MEDIA_CHARACTERISTICS_COUNT][PICO_M3U8_MAX_MEDIA_CHARACTERISTICS_LENGTH];
    uint8_t characteristicsCount;

    
    // The value is a quoted-string that specifies an ordered, backslash-
    // separated ("/") list of parameters.  If the TYPE attribute is
    // AUDIO, then the first parameter is a count of audio channels
    // expressed as a decimal-integer, indicating the maximum number of
    // independent, simultaneous audio channels present in any Media
    // Segment in the Rendition.  For example, an AC-3 5.1 Rendition
    // would have a CHANNELS="6" attribute.  No other CHANNELS parameters
    // are currently defined.
    // All audio EXT-X-MEDIA tags SHOULD have a CHANNELS attribute.  If a
    // Master Playlist contains two Renditions encoded with the same
    // codec but a different number of channels, then the CHANNELS
    // attribute is REQUIRED; otherwise, it is OPTIONAL.
    char channels[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

} picoM3U8MediaAttributes_t;
typedef picoM3U8MediaAttributes_t *picoM3U8MediaAttributes;

typedef struct {
    uint32_t width;
    uint32_t height;
} picoM3U8Resolution;

typedef struct {
    // The value is a decimal-integer of bits per second.  It represents
    // the peak segment bit rate of the Variant Stream.
    // If all the Media Segments in a Variant Stream have already been
    // created, the BANDWIDTH value MUST be the largest sum of peak
    // segment bit rates that is produced by any playable combination of
    // Renditions.  (For a Variant Stream with a single Media Playlist,
    // this is just the peak segment bit rate of that Media Playlist.)
    // An inaccurate value can cause playback stalls or prevent clients
    // from playing the variant.
    // If the Master Playlist is to be made available before all Media
    // Segments in the presentation have been encoded, the BANDWIDTH
    // value SHOULD be the BANDWIDTH value of a representative period of
    // similar content, encoded using the same settings.
    // Every EXT-X-STREAM-INF tag MUST include the BANDWIDTH attribute.
    uint32_t bandwidth;

    // The value is a decimal-integer of bits per second.  It represents
    // the average segment bit rate of the Variant Stream.
    // If all the Media Segments in a Variant Stream have already been
    // created, the AVERAGE-BANDWIDTH value MUST be the largest sum of
    // average segment bit rates that is produced by any playable
    // combination of Renditions.  (For a Variant Stream with a single
    // Media Playlist, this is just the average segment bit rate of that
    // Media Playlist.)  An inaccurate value can cause playback stalls or
    // prevent clients from playing the variant.
    // If the Master Playlist is to be made available before all Media
    // Segments in the presentation have been encoded, the AVERAGE-
    // BANDWIDTH value SHOULD be the AVERAGE-BANDWIDTH value of a
    // representative period of similar content, encoded using the same
    // settings.
    // The AVERAGE-BANDWIDTH attribute is OPTIONAL.
    uint32_t averageBandwidth;

    // The value is a quoted-string containing a comma-separated list of
    // formats, where each format specifies a media sample type that is
    // present in one or more Renditions specified by the Variant Stream.
    // Valid format identifiers are those in the ISO Base Media File
    // Format Name Space defined by "The 'Codecs' and 'Profiles'
    // Parameters for "Bucket" Media Types" [RFC6381].
    // For example, a stream containing AAC low complexity (AAC-LC) audio
    // and H.264 Main Profile Level 3.0 video would have a CODECS value
    // of "mp4a.40.2,avc1.4d401e".
    // Every EXT-X-STREAM-INF tag SHOULD include a CODECS attribute.
    char codecs[PICO_M3U8_MAX_STREAM_CODECS][PICO_M3U8_MAX_STRING_ITEM_LENGTH];
    uint8_t codecCount;


    // The value is a decimal-resolution describing the optimal pixel
    // resolution at which to display all the video in the Variant
    // Stream.
    // The RESOLUTION attribute is OPTIONAL but is recommended if the
    // Variant Stream includes video.
    picoM3U8Resolution resolution;

    // The value is a decimal-floating-point describing the maximum frame
    // rate for all the video in the Variant Stream, rounded to three
    // decimal places.
    // The FRAME-RATE attribute is OPTIONAL but is recommended if the
    // Variant Stream includes video.  The FRAME-RATE attribute SHOULD be
    // included if any video in a Variant Stream exceeds 30 frames per
    // second.
    float frameRate;

    // The value is an enumerated-string; valid strings are TYPE-0 and
    // NONE.  This attribute is advisory; a value of TYPE-0 indicates
    // that the Variant Stream could fail to play unless the output is
    // protected by High-bandwidth Digital Content Protection (HDCP) Type
    // 0 [HDCP] or equivalent.  A value of NONE indicates that the
    // content does not require output copy protection.
    // Encrypted Variant Streams with different HDCP levels SHOULD use
    // different media encryption keys.
    // The HDCP-LEVEL attribute is OPTIONAL.  It SHOULD be present if any
    // content in the Variant Stream will fail to play without HDCP.
    // Clients without output copy protection SHOULD NOT load a Variant
    // Stream with an HDCP-LEVEL attribute unless its value is NONE.
    picoM3U8HDCPLevel hdcpLevel;

    // The value is a quoted-string.  It MUST match the value of the
    // GROUP-ID attribute of an EXT-X-MEDIA tag elsewhere in the Master
    // Playlist whose TYPE attribute is AUDIO.  It indicates the set of
    // audio Renditions that SHOULD be used when playing the
    // presentation.
    // The AUDIO attribute is OPTIONAL.
    char audioGroupId[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is a quoted-string.  It MUST match the value of the
    // GROUP-ID attribute of an EXT-X-MEDIA tag elsewhere in the Master
    // Playlist whose TYPE attribute is VIDEO.  It indicates the set of
    // video Renditions that SHOULD be used when playing the
    // presentation.
    // The VIDEO attribute is OPTIONAL.
    char videoGroupId[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is a quoted-string.  It MUST match the value of the
    // GROUP-ID attribute of an EXT-X-MEDIA tag elsewhere in the Master
    // Playlist whose TYPE attribute is SUBTITLES.  It indicates the set
    // of subtitle Renditions that can be used when playing the
    // presentation.
    // The SUBTITLES attribute is OPTIONAL.
    char subtitlesGroupId[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value can be either a quoted-string or an enumerated-string
    // with the value NONE.  If the value is a quoted-string, it MUST
    // match the value of the GROUP-ID attribute of an EXT-X-MEDIA tag
    // elsewhere in the Playlist whose TYPE attribute is CLOSED-CAPTIONS,
    // and it indicates the set of closed-caption Renditions that can be
    // used when playing the presentation. 
    // If the value is the enumerated-string value NONE, all EXT-X-
    // STREAM-INF tags MUST have this attribute with a value of NONE,
    // indicating that there are no closed captions in any Variant Stream
    // in the Master Playlist.  Having closed captions in one Variant
    // Stream but not another can trigger playback inconsistencies.
    // The CLOSED-CAPTIONS attribute is OPTIONAL.
    char closedCaptionsGroupId[PICO_M3U8_MAX_STRING_ITEM_LENGTH];
} picoM3U8StreamAttributes_t;
typedef picoM3U8StreamAttributes_t *picoM3U8StreamAttributes;

typedef struct {
    // The EXT-X-STREAM-INF tag specifies a Variant Stream, which is a set
    // of Renditions that can be combined to play the presentation.  The
    // attributes of the tag provide information about the Variant Stream.
    // The URI line that follows the EXT-X-STREAM-INF tag specifies a Media
    // Playlist that carries a Rendition of the Variant Stream.  The URI
    // line is REQUIRED.  Clients that do not support multiple video
    // Renditions SHOULD play this Rendition.
    picoM3U8StreamAttributes_t streamAttributes;

    bool isIFrameOnly;

    char uri[PICO_M3U8_MAX_URI_LENGTH];
} picoM3U8VariantStream_t;
typedef picoM3U8VariantStream_t *picoM3U8VariantStream;

typedef struct {
    picoM3U8PlaylistType type;

    // The EXT-X-MEDIA tag is used to relate Media Playlists that contain
    // alternative Renditions of the same content.  For
    // example, three EXT-X-MEDIA tags can be used to identify audio-only
    // Media Playlists that contain English, French, and Spanish Renditions
    // of the same presentation.  Or, two EXT-X-MEDIA tags can be used to
    // identify video-only Media Playlists that show two different camera
    // angles.
    picoM3U8MediaAttributes_t mediaAttributes;
    bool hasMediaAttributes;

    picoM3U8VariantStream_t variantStreams[PICO_M3U8_MAX_VARIANT_STREAMS];
    uint8_t variantStreamCount;    
} picoM3U8MediaRendition_t;
typedef picoM3U8MediaRendition_t *picoM3U8MediaRendition;

typedef struct {
    // The value of DATA-ID is a quoted-string that identifies a
    // particular data value.  The DATA-ID SHOULD conform to a reverse
    // DNS naming convention, such as "com.example.movie.title"; however,
    // there is no central registration authority, so Playlist authors
    // SHOULD take care to choose a value that is unlikely to collide
    // with others.  This attribute is REQUIRED.
    char dataId[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // VALUE is a quoted-string.  It contains the data identified by
    // DATA-ID.  If the LANGUAGE is specified, VALUE SHOULD contain a
    // human-readable string written in the specified language.
    char value[PICO_M3U8_MAX_STRING_ITEM_LENGTH];

    // The value is a quoted-string containing a URI.  The resource
    // identified by the URI MUST be formatted as JSON [RFC7159];
    // otherwise, clients may fail to interpret the resource.
    char uri[PICO_M3U8_MAX_URI_LENGTH];

    // The value is a quoted-string containing a language tag [RFC5646]
    // that identifies the language of the VALUE.  This attribute is
    // OPTIONAL.
    char language[PICO_M3U8_MAX_STRING_ITEM_LENGTH];
} picoM3U8SessionData_t;
typedef picoM3U8SessionData_t *picoM3U8SessionData;

typedef struct {
    picoM3U8PlaylistType type;
    
    picoM3U8CommonInfo commonInfo;

    // A Master Playlist contains one or more EXT-X-MEDIA tags that
    // describe alternative Renditions of the same content.
    picoM3U8MediaRendition mediaRenditions;
    uint8_t mediaRenditionCount;

    // A Playlist MAY contain multiple EXT-X-SESSION-DATA tags with the same
    // DATA-ID attribute.  A Playlist MUST NOT contain more than one EXT-X-
    // SESSION-DATA tag with the same DATA-ID attribute and the same
    // LANGUAGE attribute.
    picoM3U8SessionData sessionData;
    uint8_t sessionDataCount;

    // The EXT-X-SESSION-KEY tag allows encryption keys from Media Playlists
    // to be specified in a Master Playlist.  This allows the client to
    // preload these keys without having to read the Media Playlist(s)
    // first.
    // EXT-X-SESSION-KEY tags SHOULD be added if multiple Variant Streams or
    // Renditions use the same encryption keys and formats.  An EXT-X-
    // SESSION-KEY tag is not associated with any particular Media Playlist.
    // A Master Playlist MUST NOT contain more than one EXT-X-SESSION-KEY
    // tag with the same METHOD, URI, IV, KEYFORMAT, and KEYFORMATVERSIONS
    // attribute values.
    // The EXT-X-SESSION-KEY tag is optional
    picoM3U8KeyAttributes sessionKeys;
    uint8_t sessionKeyCount;
    
    // The EXT-X-INDEPENDENT-SEGMENTS tag indicates that all media samples
    // in a Media Segment can be decoded without information from other
    // segments.  It applies to every Media Segment in the Playlist. 
    // If the EXT-X-INDEPENDENT-SEGMENTS tag appears in a Master Playlist,
    // it applies to every Media Segment in every Media Playlist in the
    // Master Playlist.
    bool independentSegments;

    // The EXT-X-START tag indicates a preferred point at which to start
    // playing a Playlist.  By default, clients SHOULD start playback at
    // this point when beginning a playback session.  This tag is OPTIONAL.
    picoM3U8StartAttributes startAttributes;

} picoM3U8MasterPlaylist_t;
typedef picoM3U8MasterPlaylist_t *picoM3U8MasterPlaylist;

typedef struct {
    picoM3U8PlaylistType type;

    picoM3U8CommonInfo commonInfo;

    picoM3U8MediaSegment mediaSegments;
    uint32_t mediaSegmentCount;

    // The EXT-X-TARGETDURATION tag specifies the maximum Media Segment
    // duration.  The EXTINF duration of each Media Segment in the Playlist
    // file, when rounded to the nearest integer, MUST be less than or equal
    // to the target duration; longer segments can trigger playback stalls
    // or other errors.  It applies to the entire Playlist file.
    uint32_t targetDuration;

    // The EXT-X-MEDIA-SEQUENCE tag indicates the Media Sequence Number of
    // the first Media Segment that appears in a Playlist file.
    // If the Media Playlist file does not contain an EXT-X-MEDIA-SEQUENCE
    // tag, then the Media Sequence Number of the first Media Segment in the
    // Media Playlist SHALL be considered to be 0.  A client MUST NOT assume
    // that segments with the same Media Sequence Number in different Media
    // Playlists contain matching content.
    uint32_t mediaSequence;

    // The EXT-X-DISCONTINUITY-SEQUENCE tag allows synchronization between
    // different Renditions of the same Variant Stream or different Variant
    // Streams that have EXT-X-DISCONTINUITY tags in their Media Playlists.
    // If the Media Playlist does not contain an EXT-X-DISCONTINUITY-
    // SEQUENCE tag, then the Discontinuity Sequence Number of the first
    // Media Segment in the Playlist SHALL be considered to be 0.
    // The EXT-X-DISCONTINUITY-SEQUENCE tag MUST appear before the first
    // Media Segment in the Playlist.
    // The EXT-X-DISCONTINUITY-SEQUENCE tag MUST appear before any EXT-
    // X-DISCONTINUITY tag.
    uint32_t discontinuitySequence;

    // The EXT-X-PLAYLIST-TYPE tag provides mutability information about the
    // Media Playlist file.  It applies to the entire Media Playlist file.
    // It is OPTIONAL.  Its format is:
    // If the EXT-X-PLAYLIST-TYPE value is EVENT, Media Segments can only be
    // added to the end of the Media Playlist.  If the EXT-X-PLAYLIST-TYPE
    // value is Video On Demand (VOD), the Media Playlist cannot change.
    // If the EXT-X-PLAYLIST-TYPE tag is omitted from a Media Playlist, the
    // Playlist can be updated according to the rules in Section 6.2.1 with
    // no additional restrictions.
    picoM3U8MediaPlaylistType playlistType;

    // The EXT-X-I-FRAMES-ONLY tag indicates that each Media Segment in the
    // Playlist describes a single I-frame.  I-frames are encoded video
    // frames whose encoding does not depend on any other frame.  I-frame
    // Playlists can be used for trick play, such as fast forward, rapid
    // reverse, and scrubbing.
    bool iFramesOnly;
} picoM3U8MediaPlaylist_t;
typedef picoM3U8MediaPlaylist_t *picoM3U8MediaPlaylist;

typedef union {
    picoM3U8PlaylistType type;
    picoM3U8MasterPlaylist_t master;
    picoM3U8MediaPlaylist_t media;
} picoM3U8Playlist_t;
typedef picoM3U8Playlist_t *picoM3U8Playlist;



#endif // PICO_M3U8_H
