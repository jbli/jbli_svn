#include <glib.h>
#include <time.h>

/* Remove the __attribute__ stuff when the compiler is not GCC. */
#if !__GNUC__
#  define __attribute__(x) /**/
#endif

#define GO_UNUSED __attribute__((unused))
#define BUFFER 			8192


char *GO_VERSION = "0.1";
int piping = 0;
char *def_log_format = "%h %^ %^ [%d %^] \"%r\" %s %b \"%R\" \"%^\" \"%^\" %t";
char *def_date_format = "%d/%b/%Y:%H:%M:%S";
char *def_time_unit = "S";

#define KB (1024)
#define MB (KB * 1024)
#define GB (MB * 1024)

/* Definitions checked against declarations */
GHashTable *ht_hosts = NULL;
GHashTable *ht_referrers = NULL;
GHashTable *ht_requests = NULL;
GHashTable *ht_status_code = NULL;
GHashTable *ht_time = NULL;
GHashTable *ht_resptime = NULL;

GHashTable *ht_status_403 = NULL;
GHashTable *ht_status_404 = NULL;
GHashTable *ht_status_500 = NULL;
GHashTable *ht_status_502 = NULL;
GHashTable *ht_status_503 = NULL;
GHashTable *ht_status_504 = NULL;


/* processing time */
time_t end_proc;
time_t now;
time_t start_proc;

/* file */
char *ifile = NULL;

/* seetings */
char *date_format = NULL;
char *log_format = NULL;
char *time_unit = NULL;
char *type_ofile = NULL;


struct logger
{
   //char *agent;
   char *date;
   char *host;
   char *hour;
   char *identd;
   char *ref;
   char *req;
   char *status;
   char *userid;
   //char *domain;
   int alloc_counter;
   int counter;
   int current_module;
   int max_value;
   int total_invalid;
   int total_process;
   long long resp_size;
   double resp_time;
};

/* output */
#define MAX_CHOICES 	   300
#define OUTPUT_N 10

int iter_ctr = 0;
int line_no = 0;

#define T_HEAD " General Summary - Overall Analysed Requests - Unique totals"
#define T_F_REQUESTS "Failed Requests"
#define T_GEN_TIME   "Generation Time"
#define T_LOG        "Log"
#define T_REFERRER   "Referrers"
#define T_REQUESTS   "Total Requests"


/* header strings */
const char *status_head = " A - HTTP Status Codes";
const char *status_desc = " Top different status codes sorted by requests";

const char *status_403_head = " A.1 - httpcode 403 sorted by requests ";
const char *status_403_desc = " Forbidden -  Server is refusing to respond to it";

const char *status_404_head = " A.2 - httpcode 404 sorted by requests ";
const char *status_404_desc = " Document Not Found - Requested resource could not be found";

const char *status_500_head = " A.3 - httpcode 500 sorted by requests ";
const char *status_500_desc = " Internal Server Error";

const char *status_502_head = " A.4 - httpcode 502 sorted by requests ";
const char *status_502_desc = " Bad Gateway - Received an invalid response from the upstream";

const char *status_503_head = " A.5 - httpcode 503 sorted by requests ";
const char *status_503_desc = " Service Unavailable - The server is currently unavailable";

const char *status_504_head = " A.6 - httpcode 504 sorted by requests ";
const char *status_504_desc = " Gateway Timeout - The upstream server failed to send request";

const char *host_head = " B - Hosts";
const char *host_desc = " Top different hosts sorted by requests";

const char *req_head = " C - Requested files (Pages-URL)";
const char *req_desc =
   " Top different files requested sorted by requests - percent - bandwidth";

const char *ref_head = " D - Referrers URLs";
const char *ref_desc = " Top different referrers sorted by requests";

const char *resptime_head = " E - Response Time ";
const char *resptime_desc = " Top different time sorted by response Time";

const char *time_top_head = " E.1 - Top time ";
const char *time_top_desc = " Top different time sorted by requests";

const char *time_sort_head = " E.2 - Sorted time ";
const char *time_sort_desc = " Top different time sorted by requests";

typedef struct GOutput_ GOutput;

struct GOutput_
{
   char *data;
   int hits;
};

struct struct_holder
{
   char *data;
   int hits;
   int curr_module;
};


char *codes[][2] = {
   {"100", "Continue - Server has received the request headers"},
   {"101", "Switching Protocols - Client asked to switch protocols"},
   {"200", "OK -  The request sent by the client was successful"},
   {"201", "Created -  The request has been fulfilled and created"},
   {"202", "Accepted - The request has been accepted for processing"},
   {"203", "Non-Authoritative Information"},
   {"204", "No Content - Request is not returning any content"},
   {"205", "Reset Content - User-Agent should reset the document"},
   {"206", "Partial Content - The partial GET has been successful"},
   {"300", "Multiple Choices - Multiple options for the resource"},
   {"301", "Moved Permanently - Resource has permanently moved"},
   {"302", "Moved Temporarily (redirect)"},
   {"303", "See Other Document - The response is at a different URI"},
   {"304", "Not Modified - Resource has not been modified"},
   {"305", "Use Proxy - Can only be accessed through the proxy"},
   {"307", "Temporary Redirect - Resource temporarily moved"},
   {"400", "Bad Request - The syntax of the request is invalid"},
   {"401", "Unauthorized - Request needs user authentication"},
   {"402", "Payment Required"},
   {"403", "Forbidden -  Server is refusing to respond to it"},
   {"404", "Document Not Found - Requested resource could not be found"},
   {"405", "Method Not Allowed - Request method not supported"},
   {"406", "Not Acceptable"},
   {"407", "Proxy Authentication Required"},
   {"408", "Request Timeout - The server timed out waiting for the request"},
   {"409", "Conflict - Conflict in the request"},
   {"410", "Gone - Resource requested is no longer available"},
   {"411", "Length Required - Invalid Content-Length"},
   {"412", "Precondition Failed - Server does not meet preconditions"},
   {"413", "Requested Entity Too Long"},
   {"414", "Requested Filename Too Long"},
   {"415", "Unsupported Media Type - Media type is not supported"},
   {"416", "Requested Range Not Satisfiable - Cannot supply that portion"},
   {"417", "Expectation Failed"},
   {"500", "Internal Server Error"},
   {"501", "Not Implemented"},
   {"502", "Bad Gateway - Received an invalid response from the upstream"},
   {"503", "Service Unavailable - The server is currently unavailable"},
   {"504", "Gateway Timeout - The upstream server failed to send request"},
   {"505", "HTTP Version Not Supported"}
};
