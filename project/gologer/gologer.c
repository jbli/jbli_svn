#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <glib.h>
#include <locale.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>  

#include "gologer.h"

static struct logger *logger;

static struct logger *
init_struct (void)
{
   struct logger *logger;

   if ((logger = malloc (sizeof (*logger))) == NULL)
      return NULL;

   logger->alloc_counter = 0;
   logger->counter = 0;
   logger->current_module = 1;
   logger->resp_size = 0LL;
   logger->total_invalid = 0;
   logger->total_process = 0;

   return logger;
}

/* frees the memory allocated by the GHashTable */
static void
free_key_value (gpointer old_key, GO_UNUSED gpointer old_value,
                GO_UNUSED gpointer user_data)
{
   g_free (old_key);
}

static void
cmd_help (void)
{
   printf ("\ngologer - %s\n\n", GO_VERSION);
   printf ("Usage: \n");
   printf ("  ./gologer log_file \n");
   printf ("  or\n");
   printf ("  cat log_file | ./gologer \n\n");

   printf ("The following options can also be supplied to the command:\n\n");
   printf ("  -f <argument>   log_format, such as: %s\n", "\"%h %^ %^ [%d %^] \"%r\" %s %b \"%R\" \"%^\" \"%^\" %t\"");
   printf ("  -d <argument>   date_format, such as: %s \n", "\"%Y-%m-%d\" or \"%d/%b/%Y:%H:%M:%S\"");
   printf ("  -u <argument>   S or M or H .\n");
   printf ("  -t <argument>   html \n");
   printf ("  -h              help \n");
   printf ("  ./gologer [ -f log_format][ -d date_format ][ -u time_unit ] [-t html]  log_file \n\n");
   printf ("\n\n");
   exit (EXIT_FAILURE);
}

void
error_handler (const char *func, char *file, int line, char *msg)
{

   fprintf (stderr, "\nGoAccess - version %s - %s %s\n", GO_VERSION,
            __DATE__, __TIME__);
   fprintf (stderr, "\nAn error has occurred");
   fprintf (stderr, "\nError occured at: %s - %s - %d", file, func, line);
   fprintf (stderr, "\nMessage: %s\n\n", msg);
   exit (EXIT_FAILURE);
}

static void
free_logger (struct logger *log)
{
   if (log->date != NULL)
      free (log->date);
   if (log->host != NULL)
      free (log->host);
   if (log->ref != NULL)
      free (log->ref);
   if (log->req != NULL)
      free (log->req);
   if (log->status != NULL)
      free (log->status);
}
char *
alloc_string (const char *str)
{
   char *new = malloc (strlen (str) + 1);
   if (new == NULL)
      error_handler (__PRETTY_FUNCTION__,
                     __FILE__, __LINE__, "Unable to allocate memory");
   strcpy (new, str);
   return new;
}

int
invalid_ipaddr (char *str)
{
   if (str == NULL || *str == '\0')
      return 1;

   union
   {
      struct sockaddr addr;
      struct sockaddr_in6 addr6;
      struct sockaddr_in addr4;
   } a;
   memset (&a, 0, sizeof (a));
   if (1 == inet_pton (AF_INET, str, &a.addr4.sin_addr))
      return 0;
   else if (1 == inet_pton (AF_INET6, str, &a.addr6.sin6_addr))
      return 0;
   return 1;
}

static char *
parse_req (char *line)
{
   char *reqs, *req_l = NULL, *req_r = NULL, *req_t = NULL, *lookfor = NULL;
   ptrdiff_t req_len = 0;

   if ((lookfor = "GET ", req_l = strstr (line, lookfor)) != NULL ||
       (lookfor = "POST ", req_l = strstr (line, lookfor)) != NULL ||
       (lookfor = "HEAD ", req_l = strstr (line, lookfor)) != NULL ||
       (lookfor = "get ", req_l = strstr (line, lookfor)) != NULL ||
       (lookfor = "post ", req_l = strstr (line, lookfor)) != NULL ||
       (lookfor = "head ", req_l = strstr (line, lookfor)) != NULL) {

      /* didn't find it - weird */
      if ((req_r = strstr (line, " HTTP/1.0")) == NULL &&
          (req_r = strstr (line, " HTTP/1.1")) == NULL)
         return alloc_string ("-");

      req_l += strlen (lookfor);

	  if((req_t = strstr (line, "?")) == NULL){
			req_len = req_r - req_l;
	  }else{
		  req_len = req_t - req_l;
	  }

      /* make sure we don't have some weird requests */
      if (req_len <= 0)
         return alloc_string ("-");

      reqs = malloc (req_len + 1);
      strncpy (reqs, req_l, req_len);
      (reqs)[req_len] = 0;
   } else
      reqs = alloc_string (line);

   return reqs;
}


char *
trim_str (char *str)
{
   char *p;
   if (!str)
      return NULL;
   if (!*str)
      return str;
   for (p = str + strlen (str) - 1; (p >= str) && isspace (*p); --p);
   p[1] = '\0';
   return str;
}

static char *
parse_string (char **str, char end)
{
   char *pch = *str, *p;
   do {
      if (*pch == end || *pch == '\0') {
         size_t len = (pch - *str + 1);
         p = malloc (len);
         if (p == NULL)
            error_handler (__PRETTY_FUNCTION__, __FILE__, __LINE__,
                           "Unable to allocate memory");
         memcpy (p, *str, (len - 1));
         p[len - 1] = '\0';
         *str += len - 1;
         return trim_str (p);
      }
      /* advance to the first unescaped delim */
      if (*pch == '\\')
         pch++;
   } while (*pch++);
   return NULL;
}

static int
parse_format (struct logger *logger, const char *fmt, char *str)
{
   const char *p;

   if (str == NULL || *str == '\0')
      return 1;

   struct tm tm;
   memset (&tm, 0, sizeof (tm));

   int special = 0;
   for (p = fmt; *p; p++) {
      if (*p == '%') {
         special++;
         continue;
      }
      if (special && *p != '\0') {
         char *pch, *sEnd, *bEnd, *tkn = NULL, *end = NULL;
         errno = 0;
         long long bandw = 0;
		 double resp_time;
		 char *req_l = NULL;
		 ptrdiff_t req_len = 0;

         switch (*p) {
           case 'd':
             if (logger->date)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                return 1;
             end = strptime (tkn, date_format, &tm);
             if (end == NULL || *end != '\0') {
                free (tkn);
                return 1;
             }
             logger->date = tkn;
             break;
          case 'h':
             if (logger->host)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                return 1;
             if (invalid_ipaddr (tkn)) {
                free (tkn);
                return 1;
             }
             logger->host = tkn;
             break;
		 case 'r':
             if (logger->req)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                return 1;
             logger->req = parse_req (tkn);
			 
             free (tkn);
             break;
	  case 'b':
             if (logger->resp_size)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                return 1;
             bandw = strtol (tkn, &bEnd, 10);
             if (tkn == bEnd || *bEnd != '\0' || errno == ERANGE)
                bandw = 0;
             logger->resp_size = bandw;
             free (tkn);
             break;
          case 's':
             if (logger->status)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                return 1;
             strtol (tkn, &sEnd, 10);
             if (tkn == sEnd || *sEnd != '\0' || errno == ERANGE) {
                free (tkn);
                return 1;
             }
             logger->status = tkn;
             break;
          case 'R':
             if (logger->ref)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                tkn = alloc_string ("-");
             if (tkn != NULL && *tkn == '\0') {
                free (tkn);
                tkn = alloc_string ("-");
             }
			 if( strcmp(tkn, "-") && ((req_l = strstr (tkn, "?")) != NULL)){
				  req_len = req_l - tkn;
				  tkn[req_len] = 0;
			 }
		     logger->ref = tkn;	
             break;
/*
          case 'u':
             if (logger->agent)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                tkn = alloc_string ("-");
             if (tkn != NULL && *tkn == '\0') {
                free (tkn);
                tkn = alloc_string ("-");
             }
             logger->agent = tkn;
             break;
		   case 'H':
             if (logger->domain)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                tkn = alloc_string ("-");
             if (tkn != NULL && *tkn == '\0') {
                free (tkn);
                tkn = alloc_string ("-");
             }
             logger->domain = tkn;

             break;
*/
	  case 't':
             if (logger->resp_time)
                return 1;
             tkn = parse_string (&str, p[1]);
             if (tkn == NULL)
                return 1;
             resp_time = strtod (tkn, &bEnd);
             if (tkn == bEnd || *bEnd != '\0' || errno == ERANGE)
                resp_time = 0;
             logger->resp_time = resp_time;
             free (tkn);
             break;
          default:
             if ((pch = strchr (str, p[1])) != NULL)
                str += pch - str;
         }
         if ((str == NULL) || (*str == '\0'))
            return 0;
         special = 0;
      } else if (special && isspace (p[0])) {
         return 1;
      } else
         str++;
   }
   return 0;
}

static int
process_generic_data (GHashTable * ht, const char *key)
{
   gint value_int;
   gpointer value_ptr;

   if ((ht == NULL) || (key == NULL))
      return (EINVAL);
   value_ptr = g_hash_table_lookup (ht, key);
   if (value_ptr != NULL)
      value_int = GPOINTER_TO_INT (value_ptr);
   else {
      value_int = 0;
   }
   value_int++;
   /* replace the entry. old key will be freed by "free_key_value". */
   g_hash_table_replace (ht, g_strdup (key), GINT_TO_POINTER (value_int));
   return (0);
}

static int
process_log (struct logger *logger, char *line, int test)
{
   struct logger log;

   struct tm tm;
   char buf[32];
   char time_key[512];
   char date_str[256];
   int len =0;
   float tmp_f =0;

   /* make compiler happy */
   memset (buf, 0, sizeof (buf));
   memset (&log, 0, sizeof (log));
   memset (&tm, 0, sizeof (tm));
   memset (date_str, 0, sizeof (date_str));

   if (date_format == NULL || *date_format == '\0'){
      date_format = def_date_format;
      //error_handler (__PRETTY_FUNCTION__, __FILE__, __LINE__, "No date format was found on your conf file.");
   }
   if (log_format == NULL || *log_format == '\0'){
      log_format = def_log_format;
      //error_handler (__PRETTY_FUNCTION__, __FILE__, __LINE__,"No log format was found on your conf file.");
   }
  if (time_unit == NULL || *time_unit == '\0'){
      time_unit = def_time_unit;
   }

   if ((line == NULL) || (*line == '\0')) {
      logger->total_invalid++;
      return 0;
   }
   if (*line == '#' || *line == '\n')
      return 0;

   logger->total_process++;
   if (parse_format (&log, log_format, line) == 1) {
      free_logger (&log);
      logger->total_invalid++;
      return 0;
   }
   
   if(line_no < 10){
		 printf("line%d: %s", line_no, line);
		 printf("date:%s\n", log.date);
		 printf("host:%s\n", log.host);
		 printf("request:%s\n", log.req);
		 printf("respone size:%lld\n", log.resp_size);
		 printf("status:%s\n", log.status);
	     printf("refer:%s\n", log.ref);
		 //printf("agent:%s\n", log.agent);
		 //printf("domain:%s\n", log.domain);
		 printf("response time:%lf ms\n", (log.resp_time*1000));

   }
   process_generic_data (ht_hosts, log.host);
   process_generic_data (ht_status_code, log.status);
   process_generic_data (ht_requests, log.req);

   switch(atoi(log.status)){
   case 403:
     process_generic_data (ht_status_403, log.req); break;
   case 404:
     process_generic_data (ht_status_404, log.req); break;
   case 500:
     process_generic_data (ht_status_500, log.req); break;
   case 502:
     process_generic_data (ht_status_502, log.req); break;
   case 503:
     process_generic_data (ht_status_503, log.req); break;
   case 504:
     process_generic_data (ht_status_504, log.req); break;
   }

   /*if (strptime (log.date, date_format, &tm) != NULL){
	   snprintf (time_key, sizeof (time_key), "%s-%s-%s %s:%s:%s",
             tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		time_key[sizeof (time_key) - 1] = 0;
		process_generic_data (ht_time, time_key);
   }*/	
   len = strlen(log.date);
   if(!strcmp(time_unit,"H")){
		strncpy (date_str, log.date, len -6);
		date_str[len -6+1] = 0;
		process_generic_data (ht_time, date_str);
   }else if(!strcmp(time_unit,"M")){
   		strncpy (date_str, log.date, len -3);
		date_str[len -3+1] = 0;
		process_generic_data (ht_time, date_str);
   }else if(!strcmp(time_unit,"S")){
		process_generic_data (ht_time, log.date);
   }else{
		process_generic_data (ht_time, log.date);
   }

   process_generic_data (ht_referrers, log.ref);
   if(log.resp_time >=0){
	   tmp_f = log.resp_time*1000;
	   if((tmp_f >= 0)&&(tmp_f <=10)){
			process_generic_data (ht_resptime, "0~10(ms)");
	   }else if((tmp_f >10)&&(tmp_f <=100)){
			process_generic_data (ht_resptime, "10~100(ms)");
	   }else if((tmp_f >100)&&(tmp_f <=200)){
		   process_generic_data (ht_resptime, "100~200(ms)");
	   }else if((tmp_f >200)&&(tmp_f <=300)){
		   process_generic_data (ht_resptime, "200~300(ms)");
	   }else if((tmp_f >300)&&(tmp_f <=500)){
		   process_generic_data (ht_resptime, "300~500(ms)");
	   }else if((tmp_f >500)&&(tmp_f <=1000)){
		   process_generic_data (ht_resptime, "500~1000(ms)");
	   }else if((tmp_f >1000)&&(tmp_f <=1500)){
		   process_generic_data (ht_resptime, "1000~1500(ms)");
	   }else if((tmp_f >1500)&&(tmp_f <=2000)){
		   process_generic_data (ht_resptime, "1500~2000(ms)");
	   }else if((tmp_f >2000)&&(tmp_f <=3000)){
		   process_generic_data (ht_resptime, "2000~3000(ms)");
	   }else if((tmp_f >3000)&&(tmp_f <=5000)){
		   process_generic_data (ht_resptime, "3000~5000(ms)");
	   }else{
		   process_generic_data (ht_resptime, "5000~ (ms)");
	   }
   }else{
		process_generic_data (ht_resptime, "invalid");
   }
   free_logger (&log); 
   return 0;
}

int
parse_log (struct logger *logger, char *filename, char *tail, int n)
{
   FILE *fp = NULL;
   char line[BUFFER];
   int i = 0, test = 10;

   if (tail != NULL) {
      if (process_log (logger, tail, test))
         return 1;
      return 0;
   }
   if (filename == NULL) {
      fp = stdin;
      piping = 1;
   }
   if (!piping && (fp = fopen (filename, "r")) == NULL)
      error_handler (__PRETTY_FUNCTION__, __FILE__, __LINE__,
                     "An error has occurred while opening the log file. Make sure it exists.");

   while (fgets (line, BUFFER, fp) != NULL) {
	   line_no ++;
      if (n >= 0 && i++ == n)
         break;
      if (process_log (logger, line, test)) {
         if (!piping)
            fclose (fp);
         return 1;
      }
   }
   /* definitely not portable! */
   if (piping)
      stdin = freopen ("/dev/tty", "r", stdin);

   if (!piping)
      fclose (fp);
   return 0;
}

/* output */
static void
print_html_header (FILE *fp, char *now)
{
   fprintf (fp, "<html>\n");
   fprintf (fp, "<head>\n");
   fprintf (fp, "<title>Server Statistics - %s</title>\n", now);
   fprintf (fp, "<script type=\"text/javascript\">\n");

   fprintf (fp,
   "function t(c){for(var "
   "b=c.parentNode.parentNode.parentNode.getElementsByTagName('tr'),"
   "a=0;a<b.length;a++)'hide'==b[a].className?(b[a].className='show',"
   "c.innerHTML='[-] Collapse'):'show'=="
   "b[a].className&&(b[a].className='hide',c.innerHTML='[+] Expand')};");

   fprintf (fp,
   "function a(c){var b=c.parentNode.parentNode.nextElementSibling;"
   "'a-hide'==b.className?(b.className='a-show',c.innerHTML='[-]'):"
   "'a-show'==b.className&&(b.className='a-hide',c.innerHTML='[+]')};");

   fprintf (fp, "</script>\n");
   fprintf (fp, "<style type=\"text/css\">\n");

   fprintf (fp,
   "body {"
   "   font-family: Verdana;"
   "   font-size: 11px;"
   "}"
   "table.a1,"
   "table.a2 {"
   "   border-spacing: 0;"
   "   font-size: 11px;"
   "   margin: 5px;"
   "   table-layout: fixed;"
   "   white-space: nowrap;"
   "}"
   "table.a1 { width: 600px }"
   "table.a2 {"
   "   background-color: #EEE;"
   "   padding: 0 5px;"
   "   width: 590px;"
   "}"
   ".head {"
   "   background-color: #222;"
   "   color: #FFF;"
   "   padding: 5px;"
   "}"
   ".head span,"
   ".s {"
   "   cursor: pointer;"
   "   font-size: 9px;"
   "}"
   ".r { text-align: right }"
   ".red {"
   "   color: #D20B2C;"
   "   font-weight: 700;"
   "}"
   ".lnk {"
   "   font-weight:bold;"
   "   font-size:10px;"
   "}"
   "a { color: #222 }"
   ".desc {"
   "   background-color: #EEE;"
   "   color: #222;"
   "   padding: 5px;"
   "}"
   ".d1,"
   ".d2 {"
   "   overflow: hidden;"
   "   text-overflow: ellipsis;"
   "}"
   ".d1 { border-bottom: 1px dotted #eee }"
   ".d2 { border-bottom: 1px dotted #ccc }"
   ".bar {"
   "   background-color: #777;"
   "   border-right: 1px #FFF solid;"
   "   height: 10px;"
   "}"
   ".a-hide,"
   ".hide { display: none }");

   fprintf (fp, "</style>\n");
   fprintf (fp, "</head>\n");
   fprintf (fp, "<body>\n");
}

static void
print_html_begin_table (FILE * fp)
{
   fprintf (fp, "<table class=\"a1\">\n");
}

static void
print_html_end_table (FILE * fp)
{
   fprintf (fp, "</table>\n");
}

static void
print_html_head_top (FILE * fp, const char *s, int span, int exp)
{
   fprintf (fp, "<tr>");
   if (exp) {
      span--;
      fprintf (fp, "<td class=\"head\" colspan=\"%d\">%s</td>", span, s);
      fprintf (fp, "<td class=\"head r\">");
      fprintf (fp, "<span onclick=\"t(this)\">Expand [+]</span>");
      fprintf (fp, "</td>");
   } else
      fprintf (fp, "<td class=\"head\" colspan=\"%d\">%s</td>", span, s);
   fprintf (fp, "</tr>\n");
}

static void
print_html_head_bottom (FILE * fp, const char *s, int colspan)
{
   fprintf (fp, "<tr>");
   fprintf (fp, "<td class=\"desc\" colspan=\"%d\">%s</td>", colspan, s);
   fprintf (fp, "</tr>\n");
}

static void
print_html_col (FILE * fp, int w)
{
   fprintf (fp, "<col style=\"width:%dpx\">\n", w);
}

static void
print_html_begin_tr (FILE * fp, int hide)
{
   if (hide)
      fprintf (fp, "<tr class=\"hide\">");
   else
      fprintf (fp, "<tr>");
}

static void
print_html_end_tr (FILE * fp)
{
   fprintf (fp, "</tr>");
}

/* off_t becomes 64 bit aware */
off_t
file_size (const char *filename)
{
   struct stat st;

   if (stat (filename, &st) == 0)
      return st.st_size;

   fprintf (stderr, "Cannot determine size of %s: %s\n", filename,
            strerror (errno));

   return -1;
}

char *
filesize_str (off_t log_size)
{
   char *size = (char *) malloc (sizeof (char) * 10);
   if (size == NULL)
      error_handler (__PRETTY_FUNCTION__,
                     __FILE__, __LINE__, "Unable to allocate memory");
   if (log_size >= GB)
      snprintf (size, 10, "%.2f GB", (double) (log_size) / GB);
   else if (log_size >= MB)
      snprintf (size, 10, "%.2f MB", (double) (log_size) / MB);
   else if (log_size >= KB)
      snprintf (size, 10, "%.2f KB", (double) (log_size) / KB);
   else
      snprintf (size, 10, "%.1f  B", (double) (log_size));

   return size;
}

static void
print_html_summary_field (FILE * fp, int hits, char *field)
{
   fprintf (fp, "<td class=\"d1\">%s</td>", field);
   fprintf (fp, "<td class=\"d1\">%d</td>", hits);
}

static void
print_summary (FILE * fp, struct logger *logger, int flag)
{
   char *bw, *size;
   off_t log_size;
   if (flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 180);
       print_html_col (fp, 180);
       print_html_col (fp, 180);
       print_html_col (fp, 180);
       print_html_col (fp, 180);
       print_html_col (fp, 180);
       print_html_head_top (fp, T_HEAD, 8, 0);
   }else{
       fprintf (fp, "%s\n", T_HEAD);
   }

   if (!piping) {
      log_size = file_size (ifile);
      size = filesize_str (log_size);
   } else
      size = alloc_string ("N/A");

   if (ifile == NULL)
      ifile = "STDIN";
  
   if (flag == 1){
       print_html_begin_tr (fp, 0);
       fprintf (fp, "<td class=\"d1\">%s</td>", "Filename:");
       fprintf (fp, "<td class=\"d1\">%s</td>", ifile);
       fprintf (fp, "<td class=\"d1\">%s</td>", "Filesize:");
       fprintf (fp, "<td class=\"d1\">%s</td>", size);
       print_html_end_tr (fp);

       print_html_begin_tr (fp, 0);
       print_html_summary_field (fp, logger->total_process, T_REQUESTS);
       print_html_summary_field (fp, logger->total_invalid, T_F_REQUESTS);
       print_html_end_tr (fp);

       print_html_begin_tr (fp, 0);
       fprintf (fp, "<td class=\"d1\">%s</td>", T_GEN_TIME);
       fprintf (fp, "<td class=\"d1\">%lu</td>", ((int) end_proc - start_proc));
       print_html_end_tr (fp);

       print_html_end_table (fp);
   }else{
       fprintf (fp, "Filename: %s    Filesize: %s\n", ifile, size);
       fprintf (fp, "Total Requests: %d    Failed Requests: %d\n", logger->total_process, logger->total_invalid);
       fprintf (fp, "Generation Time: %lu\n", ((int) end_proc - start_proc));
       fprintf (fp, "\n\n");
   }
   free (size);
}

report_iter (gpointer k, gpointer v, GOutput ** output)
{
   output[iter_ctr] = malloc (sizeof (GOutput));
   output[iter_ctr]->data = (gchar *) k;
   output[iter_ctr++]->hits = GPOINTER_TO_INT (v);
}
/* qsort struct comparision function (hits field) */
int
struct_cmp_by_hits (const void *a, const void *b)
{
   struct struct_holder *ia = *(struct struct_holder **) a;
   struct struct_holder *ib = *(struct struct_holder **) b;
   return (int) (ib->hits - ia->hits);
   /* integer comparison: returns negative if b > a and positive if a > b */
}

int
struct_cmp_by_digit (const void *a, const void *b)
{
   int m = 0, n = 0 ;
   struct struct_holder *ia = *(struct struct_holder **) a;
   struct struct_holder *ib = *(struct struct_holder **) b;
   if((*(ia->data)<'0')||(*(ia->data)>'9')){
		m = 10000;
   }
   if((*(ib->data)<'0')||(*(ib->data)>'9')){
		n = 10000;
   }

   m = atoi(ia->data);
   n = atoi(ib->data);
   return (int)(m - n);
   /* integer comparison: returns negative if b > a and positive if a > b */
}

struct_cmp_asc (const void *a, const void *b)
{
   struct struct_holder *ia = *(struct struct_holder **) a;
   struct struct_holder *ib = *(struct struct_holder **) b;
   return strcmp (ia->data, ib->data);
}

static GOutput **
new_goutput (int n)
{
   GOutput **output = malloc (sizeof (GOutput *) * n);
   if (output == NULL)
      error_handler (__PRETTY_FUNCTION__, __FILE__, __LINE__,
                     "Unable to allocate memory for new GOutput.");
   memset (output, 0, sizeof *output);

   return output;
}

static void
print_hosts (FILE * fp, struct logger *logger, int flag)
{
   GHashTable *ht = ht_hosts;
   char  *v = NULL, *ptr_value;
   double t, l;
   int n = g_hash_table_size (ht), i, k, max, j, until = 0, delims = 0;
   size_t alloc = 0;

   iter_ctr = 0;
   GOutput **output = new_goutput (n);
   g_hash_table_foreach (ht, (GHFunc) report_iter, output);

   qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_by_hits);

   if (flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 20);
       print_html_col (fp, 60);
       print_html_col (fp, 80);
       print_html_col (fp, 120);
       print_html_col (fp, 80);
       print_html_col (fp, 240);

       print_html_head_top (fp, host_head, 6, n > OUTPUT_N ? 1 : 0);
       print_html_head_bottom (fp, host_desc, 6);
   }else{
        fprintf (fp, "%s %s\n", host_head, host_desc);
   }

   until = n < MAX_CHOICES ? n : MAX_CHOICES;
   max = 0;
   for (i = 0; i < until; i++) {
      if (output[i]->hits > max)
         max = output[i]->hits;
   }
   for (i = 0; i < until; i++) {
      k = output[i]->hits;
      v = output[i]->data;

      t = ((double) (k) / logger->total_process)*100;
      l = ((double) (k) / max)*100;
      l = l < 1 ? 1 : l;
      
      if (flag == 1){
          print_html_begin_tr (fp, i > OUTPUT_N ? 1 : 0);
          fprintf (fp, "<td>");
          fprintf (fp, "<span class=\"s\">-</span>");
          fprintf (fp, "</td>");

          fprintf (fp, "<td class=\"d1\">%d</td>", k);
          fprintf (fp, "<td class=\"d1\">%4.2lf%%</td>", t);
          fprintf (fp, "<td class=\"d1\">%s</td>", v);

          fprintf (fp, "<td class=\"d1\">");
          fprintf (fp, "<div class=\"bar\" style=\"width:%lf%%\"></div>", l);
          fprintf (fp, "</td>");
          print_html_end_tr (fp);
       }else{
          fprintf (fp, "%d    %4.2lf%%    %s\n", k, t, v);
       }    
   }

   for (i = 0; i < n; i++)
      free (output[i]);
   free (output);
   if (flag == 1)
       print_html_end_table (fp);
   else
       fprintf (fp, "\n\n");
}

static void
print_request_report (FILE * fp, GHashTable * ht, struct logger *logger, int flag)
{
   char *bw, *v = NULL;
   double t;
   int n = g_hash_table_size (ht), i, k, until = 0;

   if (n == 0)
      return;

   iter_ctr = 0;
   GOutput **output = new_goutput (n);
   g_hash_table_foreach (ht, (GHFunc) report_iter, output);
   qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_by_hits);

   if (flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 60);
       print_html_col (fp, 80);
       print_html_col (fp, 360);
   }

   if (ht == ht_requests) {
      if (flag == 1){
          print_html_head_top (fp, req_head, 4, n > OUTPUT_N ? 1 : 0);
          print_html_head_bottom (fp, req_desc, 4);
      }else{
          fprintf (fp, "%s %s\n", req_head, req_desc);
      }
   } else if (ht == ht_referrers) {
      if (flag == 1){
          print_html_head_top (fp, ref_head, 4, n > OUTPUT_N ? 1 : 0);
          print_html_head_bottom (fp, ref_desc, 4);
      }else{
          fprintf (fp, "%s %s\n", ref_head, ref_desc);
      }
   }

   until = n < MAX_CHOICES ? n : MAX_CHOICES;
   for (i = 0; i < until; i++) {
      k = output[i]->hits;
      v = output[i]->data;

      t = ((double) (k) / logger->total_process)*100;
      if (flag == 1){
          print_html_begin_tr (fp, i > OUTPUT_N ? 1 : 0);
          fprintf (fp, "<td class=\"d1\">%d</td>", k);
          fprintf (fp, "<td class=\"d1\">%4.2lf%%</td>", t);
          fprintf (fp, "<td class=\"d1\">%s</td>", v);
          print_html_end_tr (fp);
       }else{
          fprintf (fp, "%d    %4.2lf%%    %s\n", k, t, v);
       }  
   }
   for (i = 0; i < n; i++)
      free (output[i]);

   free (output);
   if (flag == 1)
       print_html_end_table (fp);
   else
       fprintf (fp, "\n\n");
}

static void
print_time_top (FILE * fp, GHashTable * ht, struct logger *logger, int flag)
{
   char *bw, *v = NULL;
   double t;
   int n = g_hash_table_size (ht), i, k, until = 0;

   if (n == 0)
      return;

   iter_ctr = 0;
   GOutput **output = new_goutput (n);
   g_hash_table_foreach (ht, (GHFunc) report_iter, output);

   qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_by_hits);
   
   if ( flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 60);
       print_html_col (fp, 80);
       print_html_col (fp, 240);
       print_html_head_top (fp, time_top_head, 4, n > OUTPUT_N ? 1 : 0);
       print_html_head_bottom (fp, time_top_desc, 4);
   }else{
        fprintf (fp, "%s %s\n", time_top_head, time_top_desc);
   }
 
   until = n < MAX_CHOICES ? n : MAX_CHOICES;
   for (i = 0; i < until; i++) {
      k = output[i]->hits;
      v = output[i]->data;

      t = ((double) (k) / logger->total_process)*100;
      if (flag == 1){
          print_html_begin_tr (fp, i > OUTPUT_N ? 1 : 0);
          fprintf (fp, "<td class=\"d1\">%d</td>", k);
          fprintf (fp, "<td class=\"d1\">%4.2lf%%</td>", t);
          fprintf (fp, "<td class=\"d1\">%s</td>", v);
          print_html_end_tr (fp);
      }else{
          fprintf (fp, "%d    %4.2lf%%    %s\n", k, t, v);
      }

   }
   for (i = 0; i < n; i++)
      free (output[i]);

   free (output);
   if (flag == 1)
       print_html_end_table (fp);
   else
       fprintf (fp, "\n\n");
}


static void
print_time_sort (FILE * fp, GHashTable * ht, struct logger *logger, int flag)
{
   char *v = NULL;
   double t;
   int n = g_hash_table_size (ht), i, k, until = 0;

   if (n == 0)
      return;

   iter_ctr = 0;
   GOutput **output = new_goutput (n);
   g_hash_table_foreach (ht, (GHFunc) report_iter, output);

   if(ht == ht_time){
       qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_asc);
   }else if(ht == ht_resptime){
       qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_by_digit);
   }

   if (flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 60);
       print_html_col (fp, 80);
       print_html_col (fp, 240);
   }

   if (ht == ht_time){
       if (flag == 1){
           print_html_head_top (fp, time_sort_head, 4, n > OUTPUT_N ? 1 : 0);
	   print_html_head_bottom (fp, time_sort_desc, 4);
       }else{
           fprintf (fp, "%s %s\n", time_sort_head, time_sort_desc);
       }     

   }else if(ht == ht_resptime){
       if (flag == 1){
	   print_html_head_top (fp, resptime_head, 4, n > OUTPUT_N ? 1 : 0);
	   print_html_head_bottom (fp, resptime_desc, 4);
       }else{
           fprintf (fp, "%s %s\n", resptime_head, resptime_desc);
       }      
   }
 
   until = n ;
   for (i = 0; i < until; i++) {
      k = output[i]->hits;
      v = output[i]->data;

      t = ((double) (k) / logger->total_process)*100;
      if (flag == 1){
          print_html_begin_tr (fp, i > OUTPUT_N ? 1 : 0);
          fprintf (fp, "<td class=\"d1\">%d</td>", k);
          fprintf (fp, "<td class=\"d1\">%4.2lf%%</td>", t);
          fprintf (fp, "<td class=\"d1\">%s</td>", v);
          print_html_end_tr (fp);
      }else{
          fprintf (fp, "%d    %4.2lf%%    %s\n", k, t, v);
      }
   }
   for (i = 0; i < n; i++)
      free (output[i]);

   free (output);
   if (flag == 1)
       print_html_end_table (fp);
   else
       fprintf (fp, "\n\n");
}

/* print_html_status */
size_t
codes_size ()
{
   return sizeof (codes) / sizeof (codes[0]);
}

char *
verify_status_code (char *str)
{
   size_t i;
   for (i = 0; i < codes_size (); i++)
      if (strstr (str, codes[i][0]) != NULL)
         return codes[i][1];
   return "Unknown";
}

static void
print_status (FILE * fp, struct logger *logger, int flag)
{
   GHashTable *ht = ht_status_code;

   char *v = NULL;
   double t;
   int n = g_hash_table_size (ht), i, k;
   if (n == 0)
      return;

   iter_ctr = 0;
   GOutput **output = new_goutput (n);
   g_hash_table_foreach (ht, (GHFunc) report_iter, output);

   qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_by_hits);
   if (flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 60);
       print_html_col (fp, 80);
       print_html_col (fp, 60);
       print_html_col (fp, 400);
       print_html_head_top (fp, status_head, 4, 0);
       print_html_head_bottom (fp, status_desc, 4);
   }else{
       fprintf (fp, "%s %s\n", status_head, status_desc);
   }

   for (i = 0; i < n; i++) {
      k = output[i]->hits;
      v = output[i]->data;

      t = ((double) (k) / logger->total_process)*100;
      if (flag == 1){
          print_html_begin_tr (fp, 0);
          fprintf (fp, "<td class=\"d1\">%d</td>", k);
          fprintf (fp, "<td class=\"d1\">%4.2lf%%</td>", t);
          fprintf (fp, "<td class=\"d1\">%s</td>", v);
          fprintf (fp, "<td class=\"d1\">%s</td>", verify_status_code (v));
          print_html_end_tr (fp);
      }else{
          fprintf (fp, "%d    %4.2lf%%    %s    %s\n", k, t, v, verify_status_code (v));
      }
      free (output[i]);
   }
   free (output);
   if (flag == 1)
       print_html_end_table (fp);
   else
       fprintf (fp, "\n\n");
}


static void
print_error_status (FILE * fp, GHashTable * ht, const char *item_head, const char *item_desc, struct logger *logger, int flag)
{
   //GHashTable *ht = ht_status_code;
   char *v = NULL;
   double t;
   int n = g_hash_table_size (ht), i, k;
   if (n == 0)
      return;

   iter_ctr = 0;
   GOutput **output = new_goutput (n);
   g_hash_table_foreach (ht, (GHFunc) report_iter, output);

   qsort (output, iter_ctr, sizeof (GOutput *), struct_cmp_by_hits);
   if (flag == 1){
       print_html_begin_table (fp);
       print_html_col (fp, 60);
       print_html_col (fp, 80);
       print_html_col (fp, 400);
       print_html_head_top (fp, item_head, 4, n > OUTPUT_N ? 1 : 0);
       print_html_head_bottom (fp, item_desc, 4);
   }else{
       fprintf (fp, "%s %s\n", item_head, item_desc);
   }

   for (i = 0; i < n; i++) {
      k = output[i]->hits;
      v = output[i]->data;

      t = ((double) (k) / logger->total_process)*100;
      if (flag == 1){
          print_html_begin_tr (fp, i > OUTPUT_N ? 1 : 0);
          fprintf (fp, "<td class=\"d1\">%d</td>", k);
          fprintf (fp, "<td class=\"d1\">%4.2lf%%</td>", t);
          fprintf (fp, "<td class=\"d1\">%s</td>", v);
          print_html_end_tr (fp);
     }else{
          fprintf (fp, "%d    %4.2lf%%    %s\n", k, t, v);
      }
      free (output[i]);
   }
   free (output);
   if (flag == 1)
       print_html_end_table (fp);
   else
       fprintf (fp, "\n\n");
}
/* free memory*/

static void
house_keeping (struct logger *logger)
{
   free (logger);

   g_hash_table_destroy (ht_hosts);
   g_hash_table_destroy (ht_referrers);
   g_hash_table_destroy (ht_requests);
   g_hash_table_destroy (ht_status_code);
   g_hash_table_destroy (ht_time);
   g_hash_table_destroy (ht_resptime);

   g_hash_table_destroy (ht_status_403);
   g_hash_table_destroy (ht_status_404);
   g_hash_table_destroy (ht_status_500);
   g_hash_table_destroy (ht_status_502);
   g_hash_table_destroy (ht_status_503);
   g_hash_table_destroy (ht_status_504);
}


int
main (int argc, char *argv[])
{
   extern char *optarg;
   extern int optind, optopt, opterr;
   int row, col, o, index, conf = 0, quit = 0;
   char ofile[1024];

   opterr = 0;
   while ((o = getopt (argc, argv, "f:d:t:u:h")) != -1) {
      switch (o) {
       case 't':
          type_ofile = optarg;
	      fprintf(stderr, "type_outfile %s \n", type_ofile);
          break;
       case 'd':
          date_format = optarg;
	      fprintf(stderr, "date_format %s \n", date_format);
          break;
       case 'f':
          log_format = optarg;
	      fprintf(stderr, "log_format %s \n", log_format);
          break;
       case 'u':
          time_unit = optarg;
	      fprintf(stderr, "time_unit %s \n", time_unit);
          break;
       case 'h':
          cmd_help ();
          break;
       case '?':
          if (optopt == 't' || optopt == 'd' || optopt == 'f' || optopt == 'u')
             fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else if (isprint (optopt))
             fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          else
             fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
          return 1;
       default:
          abort ();
      }
   }
   for (index = optind; index < argc-1; index++)
      cmd_help ();
   if (optind + 1 == argc){
     ifile = argv[argc-1];
     fprintf (stderr, "ifile %s \n", ifile);
   }
   if (!isatty (STDOUT_FILENO))
      cmd_help ();
   if (ifile != NULL && !isatty (STDIN_FILENO))
      cmd_help ();
   if (ifile == NULL && isatty (STDIN_FILENO))
      cmd_help ();


   /* initialize hash tables */
   ht_requests =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_referrers =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_hosts =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_status_code =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_time =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);

   ht_resptime =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);

   ht_status_403 =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_status_404 =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_status_500 =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_status_502 =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_status_503 =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);
   ht_status_504 =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             (GDestroyNotify) free_key_value, NULL);

    /** 'should' work on UTF-8 terminals as long as the
     ** user did set it to *._UTF-8 locale **/
    /** ###TODO: real UTF-8 support needs to be done **/
   setlocale (LC_CTYPE, "");

   logger = init_struct ();

	/* main processing event */
	(void) time (&start_proc);
    parse_log (logger, ifile, NULL, -1);
	(void) time (&end_proc);

    /* output */
    time_t now;
    time(&now);
    struct tm *now_tm = localtime (&now);
    memset(ofile, 0, sizeof(ofile));
    if( type_ofile && !strcmp(type_ofile,"html")) {
        sprintf(ofile,"result_%04d%02d%02d_%02d%02d%02d.html", now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min,now_tm->tm_sec);
	FILE *fp = fopen(ofile,"w");
	print_html_header (fp, asctime (now_tm));
	print_summary(fp, logger, 1);

	print_status(fp, logger, 1);
        print_error_status(fp, ht_status_403, status_403_head, status_403_desc, logger, 1);
        print_error_status(fp, ht_status_404, status_404_head, status_404_desc, logger, 1);
        print_error_status(fp, ht_status_500, status_500_head, status_500_desc, logger, 1);
        print_error_status(fp, ht_status_502, status_502_head, status_502_desc, logger, 1);
        print_error_status(fp, ht_status_503, status_503_head, status_503_desc, logger, 1);
        print_error_status(fp, ht_status_504, status_504_head, status_504_desc, logger, 1);

        print_hosts(fp, logger, 1);
	print_request_report (fp, ht_requests, logger, 1);
	print_request_report (fp, ht_referrers, logger, 1);

	print_time_sort(fp, ht_resptime, logger, 1);
	print_time_top(fp, ht_time, logger, 1);
	print_time_sort(fp, ht_time, logger, 1);
        fclose(fp);
     }else{
        sprintf(ofile,"result_%04d%02d%02d_%02d%02d%02d.txt", now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min,now_tm->tm_sec);
	FILE *fp = fopen(ofile,"w");
        if (fp == NULL){
          fprintf (stderr, "ofile %s cannot open! \n", ofile);
          return 1;
        }
        print_summary(fp, logger, 0);
	print_status(fp, logger, 0);
        print_error_status(fp, ht_status_403, status_403_head, status_403_desc, logger, 0);
        print_error_status(fp, ht_status_404, status_404_head, status_404_desc, logger, 0);
        print_error_status(fp, ht_status_500, status_500_head, status_500_desc, logger, 0);
        print_error_status(fp, ht_status_502, status_502_head, status_502_desc, logger, 0);
        print_error_status(fp, ht_status_503, status_503_head, status_503_desc, logger, 0);
        print_error_status(fp, ht_status_504, status_504_head, status_504_desc, logger, 0);
        print_hosts(fp, logger, 0);
        print_request_report(fp, ht_requests, logger, 0);
        print_request_report (fp, ht_referrers, logger, 0);

        print_time_sort(fp, ht_resptime, logger, 0);
	print_time_top(fp, ht_time, logger, 0);
	print_time_sort(fp, ht_time, logger, 0);
        fclose(fp);
     }
	
     house_keeping(logger);
     fprintf(stderr, "that's ok! result saved in %s\n", ofile);
     return 0;
}
