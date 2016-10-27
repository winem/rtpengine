/**
 * recording.h
 *
 * Handles call recording to PCAP files and recording metadata.
 * Mostly filesystem operations
 */

#ifndef __RECORDING_H__
#define __RECORDING_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pcap.h>
#include "str.h"
#include "aux.h"


struct packet_stream;
struct call;
enum call_opmode;


struct recording_pcap {
	FILE          *meta_fp;
	pcap_t        *recording_pd;
	pcap_dumper_t *recording_pdumper;
	uint64_t      packet_num;
	char          *recording_path;

	mutex_t       recording_lock;
};

struct recording_proc {
	unsigned int call_idx;
};
struct recording_stream_proc {
	unsigned int stream_idx;
};

struct recording {
	union {
		struct recording_pcap pcap;
		struct recording_proc proc;
	};

	str		*metadata; // from controlling daemon
	char		*escaped_callid; // call-id with dangerous characters escaped
	char		*meta_prefix; // escaped call-id plus random suffix
	char		*meta_filepath; // full file path
};

struct recording_stream {
	union {
		struct recording_stream_proc proc;
	};
};

struct recording_method {
	const char *name;
	int kernel_support;

	int (*create_spool_dir)(const char *);
	void (*init_struct)(struct call *);
	ssize_t (*write_meta_sdp)(struct recording *, struct iovec *, int, unsigned int, enum call_opmode);
	void (*dump_packet)(struct recording *, struct packet_stream *sink, str *s);
	void (*finish)(struct call *);

	void (*setup_stream)(struct packet_stream *);
};

extern const struct recording_method *selected_recording_method;

#define _rm(call, args...) selected_recording_method->call(args)
#define _rm_chk1(call, recording) do { \
		if (recording) \
			_rm(call, recording); \
	} while (0)
#define _rm_chk(call, recording, args...) do { \
		if (recording) \
			_rm(call, recording, args); \
	} while (0)



/**
 * Initialize RTP Engine filesystem settings and structure.
 * Check for or create the RTP Engine spool directory.
 */
void recording_fs_init(const char *spooldir, const char *method);

/**
 *
 * Controls the setting of recording variables on a `struct call *`.
 * Sets the `record_call` value on the `struct call`, initializing the
 * recording struct if necessary.
 * If we do not yet have a PCAP file associated with the call, create it
 * and write its file URL to the metadata file.
 *
 * Returns a boolean for whether or not the call is being recorded.
 */
int detect_setup_recording(struct call *call, str recordcall);

/**
 * Create a call metadata file in a temporary location.
 * Attaches the filepath and the file pointer to the call struct.
 * Returns path to created file.
 *
 * Metadata file format is (with trailing newline):
 *
 *     /path/to/recording-pcap.pcap
 *
 *
 *     first SDP answer
 *
 *     second SDP answer
 *
 *     ...
 *
 *     n-th and final SDP answer
 *
 *
 *     start timestamp (YYYY-MM-DDThh:mm:ss)
 *     end timestamp   (YYYY-MM-DDThh:mm:ss)
 *
 *
 *     generic metadata
 *
 * There are two empty lines between each logic block of metadata.
 * The generic metadata at the end can be any length with any number of lines.
 * Temporary files go in /tmp/. They will end up in
 * ${RECORDING_DIR}/metadata/. They are named like:
 * ${CALL_ID}-${RAND-HEX}.pcap
 *
 */
//str *meta_setup_file(struct recording *recording, str callid);

/**
 * Write out a block of SDP to the metadata file.
 */
//ssize_t meta_write_sdp(struct recording *, struct iovec *sdp_iov, int iovcnt,
//		       enum call_opmode opmode);
#define meta_write_sdp(args...) _rm_chk(write_meta_sdp, args)

/**
 * Writes metadata to metafile, closes file, and moves it to finished location.
 * Returns non-zero for failure.
 *
 * Metadata files are moved to ${RECORDING_DIR}/metadata/
 */
// int meta_finish_file(struct call *call);

/**
 * Flushes PCAP file, closes the dumper and descriptors, and frees object memory.
 */
// void recording_finish_file(struct recording *recording);

// combines the two calls above
void recording_finish(struct call *);

/**
 * Write out a PCAP packet with payload string.
 * A fair amount extraneous of packet data is spoofed.
 */
// void dump_packet(struct recording *, struct packet_stream *, str *s);
#define dump_packet(args...) _rm_chk(dump_packet, args)



#define recording_setup_stream(args...) _rm(setup_stream, args)

#endif
