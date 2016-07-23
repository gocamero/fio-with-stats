#include <netinet/tcp.h>
#include <netdb.h>

#include "fio.h"

int hcd_stat_agent_connect(const char *hostname, int portno)
{
  int sockfd, value;
  struct sockaddr_in serveraddr;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    return sockfd;

  server = gethostbyname(hostname);
  if (server == NULL) {
    return -1;
  }

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
    return -1;

  if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(int)) < 0)
    return -1;

  return sockfd;
}

void gen_json(struct buf_output* output)
{
  struct thread_data *td;
  struct thread_stat *ts;
  int i = 0;
  struct jobs_eta *je;
  size_t size;

	struct json_object *root;
  struct json_object *jobarray;
	root = json_create_object();
  json_object_add_value_string(root, "module_name", "fio");

  //ignore those data from je
  je = get_jobs_eta(true, &size);
  if (je) {

    json_object_add_value_int(root, "bw_read",    je->rate[DDIR_READ]);
    json_object_add_value_int(root, "bw_write",   je->rate[DDIR_WRITE]);
    json_object_add_value_int(root, "iops_read",  je->iops[DDIR_READ]);
    json_object_add_value_int(root, "iops_write", je->iops[DDIR_WRITE]);
    json_object_add_value_int(root, "total_jobs", je->nr_threads);
  }
  free(je);

  jobarray = json_create_object();
  json_object_add_value_array(root, "jobs", jobarray);
  
  for_each_td(td, i) {
    struct json_object *jobobj = json_create_object();
    json_array_add_value_object(jobarray, jobobj);

    ts = &td->ts;
	  json_object_add_value_int(jobobj, "groupid", td->groupid);
	  json_object_add_value_int(jobobj, "job_id", td->thread_number);
	  json_object_add_value_string(jobobj, "jobname", td->o.name);
	  json_object_add_value_string(jobobj, "runstate", runstate_to_name(td->runstate));
	  json_object_add_value_int(jobobj, "lat_read", (int)ts->lat_stat[DDIR_READ].mean.u.f);
	  json_object_add_value_int(jobobj, "lat_write", (int)ts->lat_stat[DDIR_WRITE].mean.u.f);
	  json_object_add_value_int(jobobj, "bw_read", (int)ts->bw_stat[DDIR_READ].mean.u.f);
	  json_object_add_value_int(jobobj, "bw_write", (int)ts->bw_stat[DDIR_WRITE].mean.u.f);
	  json_object_add_value_int(jobobj, "iops_read", (int)ts->iops_stat[DDIR_READ].mean.u.f);
	  json_object_add_value_int(jobobj, "iops_write", (int)ts->iops_stat[DDIR_WRITE].mean.u.f);
	  json_object_add_value_int(jobobj, "total_error", ts->total_err_count);
	  json_object_add_value_int(jobobj, "lat_r_mean", (int)td->hdr_lat_r_mean);
	  json_object_add_value_int(jobobj, "lat_r_p50", (int)td->hdr_lat_r_50);
	  json_object_add_value_int(jobobj, "lat_r_p90", (int)td->hdr_lat_r_90);
	  json_object_add_value_int(jobobj, "lat_r_p99", (int)td->hdr_lat_r_99);
	  json_object_add_value_int(jobobj, "lat_r_p9999", (int)td->hdr_lat_r_9999);
	  json_object_add_value_int(jobobj, "lat_r_max", (int)td->hdr_lat_r_max);
	  json_object_add_value_int(jobobj, "lat_w_mean", (int)td->hdr_lat_w_mean);
	  json_object_add_value_int(jobobj, "lat_w_p50", (int)td->hdr_lat_w_50);
	  json_object_add_value_int(jobobj, "lat_w_p90", (int)td->hdr_lat_w_90);
	  json_object_add_value_int(jobobj, "lat_w_p99", (int)td->hdr_lat_w_99);
	  json_object_add_value_int(jobobj, "lat_w_p9999", (int)td->hdr_lat_w_9999);
	  json_object_add_value_int(jobobj, "lat_w_max", (int)td->hdr_lat_w_max);

    td->hdr_reset = true;
  }
  json_print_object(root, output); 
  //printf("%s  %d max %d\n", output->buf, (int)output->buflen, (int)output->max_buflen);
}

static struct buf_output h_output;

void hcd_output_init(void)
{
  int len = 500000;
  h_output.buf = malloc(len);
  h_output.buflen = 0;
  h_output.max_buflen = len;
}

void hcd_stat_send(void)
{
  int ret;
  int hcdfd;
  int bytes_send;
  void *p;

  hcdfd = hcd_stat_agent_connect(stat_agent_name, stat_agent_port);
  if (hcdfd < 0)
    return;

  h_output.buflen = 0;
  gen_json(&h_output);

  ret = send(hcdfd, "w", 1, 0);
  if (ret !=1) {
    printf("Missing header 'w'\n");
  }
  
  p = h_output.buf;
  bytes_send = h_output.buflen;
  while (bytes_send > 0) {
    ret = send(hcdfd, p, bytes_send, 0); 
    if (ret <= 0) {
      printf("Error sending json obj to remote Agent\n");
      break;
    }   
    bytes_send -= ret;
    p += ret;
  }
  close(hcdfd);
}
