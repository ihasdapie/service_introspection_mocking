#include "stdio.h"

#include <assert.h>
#include <time.h>
#include <example_interfaces/srv/detail/add_two_ints__functions.h>
#include <rcl/client.h>
#include <rcl/rcl.h>
#include <rcl/service.h>
#include <rcl/error_handling.h>
#include <rcl/graph.h>
#include <rcutils/logging_macros.h>
#include <stdint.h>
#include <inttypes.h>

#include "example_interfaces/srv/add_two_ints.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

const char* ROS_PACKAGE_NAME = "example_srv_cli";
const int TAKE_TRIES = 10;


bool
wait_for_service_to_be_ready(
  rcl_service_t * service,
  rcl_context_t * context,
  size_t max_tries,
  int64_t period_ms)
{
  rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
  rcl_ret_t ret =
    rcl_wait_set_init(&wait_set, 0, 0, 0, 0, 1, 0, context, rcl_get_default_allocator());
  if (ret != RCL_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      ROS_PACKAGE_NAME, "Error in wait set init: %s", rcl_get_error_string().str);
    return false;
  }
  size_t iteration = 0;
  while (iteration < max_tries) {
    ++iteration;
    if (rcl_wait_set_clear(&wait_set) != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        ROS_PACKAGE_NAME, "Error in wait_set_clear: %s", rcl_get_error_string().str);
      return false;
    }
    if (rcl_wait_set_add_service(&wait_set, service, NULL) != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        ROS_PACKAGE_NAME, "Error in wait_set_add_service: %s", rcl_get_error_string().str);
      return false;
    }
    ret = rcl_wait(&wait_set, RCL_MS_TO_NS(period_ms));
    if (ret == RCL_RET_TIMEOUT) {
      continue;
    }
    if (ret != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error in wait: %s", rcl_get_error_string().str);
      return false;
    }
    for (size_t i = 0; i < wait_set.size_of_services; ++i) {
      if (wait_set.services[i] && wait_set.services[i] == service) {
        return true;
      }
    }
  }
  return false;
}

bool
wait_for_server_to_be_available(
  rcl_node_t * node,
  rcl_client_t * client,
  size_t max_tries,
  int64_t period_ms)
{
  size_t iteration = 0;
  while (iteration < max_tries) {
    ++iteration;
    bool is_ready;
    rcl_ret_t ret = rcl_service_server_is_available(node, client, &is_ready);
    if (ret != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        ROS_PACKAGE_NAME,
        "Error in rcl_service_server_is_available: %s",
        rcl_get_error_string().str);
      return false;
    }
    if (is_ready) {
      return true;
    }

    struct timespec tspec, tspec2;
    tspec.tv_nsec = period_ms * 1000000;
    tspec.tv_sec = 0;

    nanosleep(&tspec, &tspec2);
  }
  return false;
}




int main () {
  const char * name = "service_node";
  const rosidl_service_type_support_t * ts = ROSIDL_GET_SRV_TYPE_SUPPORT( example_interfaces, srv, AddTwoInts);
  rcl_ret_t ret; 

  // init rcl 
  rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
  ret = rcl_init_options_init(&init_options, rcl_get_default_allocator());
  assert(ret == RCL_RET_OK);

  // create a context
  rcl_context_t * context_ptr = malloc(sizeof(rcl_context_t));
  *context_ptr = rcl_get_zero_initialized_context();
  ret = rcl_init(0, NULL, &init_options, context_ptr);
  assert(ret == RCL_RET_OK);

  // make a node
  rcl_node_t * node_ptr = malloc(sizeof(rcl_node_t));
  *node_ptr = rcl_get_zero_initialized_node();
  rcl_node_options_t  node_options = rcl_node_get_default_options();

  ret = rcl_node_init(node_ptr, name, "", context_ptr, &node_options);
  assert(ret == RCL_RET_OK);

  // make a service and a client
  const char* topic = "serv_cli_example";
  rcl_service_t service = rcl_get_zero_initialized_service();
  rcl_service_options_t service_options = rcl_service_get_default_options();
  ret = rcl_service_init(&service, node_ptr, ts, topic, &service_options);
  assert(ret == RCL_RET_OK);
  printf("Created service with name: %s\n", rcl_service_get_service_name(&service));

  rcl_client_t client =rcl_get_zero_initialized_client();
  rcl_client_options_t client_options = rcl_client_get_default_options();
  ret = rcl_client_init(&client, node_ptr, ts, name, &client_options);
  assert(ret == RCL_RET_OK);
  printf("Created client for service with name: %s\n", rcl_client_get_service_name(&client));


  // wait for it to be avaliable
  ret = wait_for_server_to_be_available(node_ptr, &client, 10, 100000); // erroring out w/ 0
  printf("Server wait status: %d\n", ret);

  // make a client req and send it
  example_interfaces__srv__AddTwoInts_Request client_req;
  example_interfaces__srv__AddTwoInts_Request__init(&client_req);

  client_req.a = 1;
  client_req.b = 2;

  int64_t seq_num;

  ret = rcl_send_request(&client, &client_req, &seq_num);
  assert(ret == RCL_RET_OK);
  printf("Sent request with seq_num: %" PRIi64 "\n", seq_num);
  example_interfaces__srv__AddTwoInts_Request__fini(&client_req);

  // receive the request & respond. simulate it happening in a different scope
  ret = wait_for_service_to_be_ready(&service, context_ptr, 10, 1000); // erroring out
  printf("Service wait status: %d\n", ret);

  {
    // make a new empty request and take it
    example_interfaces__srv__AddTwoInts_Request recv_req;
    example_interfaces__srv__AddTwoInts_Request__init(&recv_req);
    rmw_service_info_t header;

    for (int i = 0; i < TAKE_TRIES; ++i) {
      ret = rcl_take_request_with_info(&service, &header, &recv_req); 
      if (ret == RCL_RET_OK) {
        printf("Received request successfully\n");
        break;
      }
    }
    assert(ret == RCL_RET_OK);
    


    printf("Received AddTwoInts Request with a=%"PRId64", b=%"PRId64"\n", recv_req.a, recv_req.b);

    // make a response and respond
    example_interfaces__srv__AddTwoInts_Response serv_resp;
    example_interfaces__srv__AddTwoInts_Response__init(&serv_resp);

    // add two ints :)
    serv_resp.sum = recv_req.a + recv_req.b;

    ret = rcl_send_response(&service, &header.request_id, &serv_resp);
    assert(ret == RCL_RET_OK);

    example_interfaces__srv__AddTwoInts_Response__fini(&serv_resp);
    example_interfaces__srv__AddTwoInts_Request__fini(&recv_req);
  }

  // receive the client response
  
  example_interfaces__srv__AddTwoInts_Response client_resp;
  example_interfaces__srv__AddTwoInts_Response__init(&client_resp);

  rmw_service_info_t header;

  for (int i = 0; i < TAKE_TRIES; ++i) {
    ret = rcl_take_response_with_info(&client, &header, &client_resp);
    if (ret == RCL_RET_OK) {
      printf("Received request successfully\n");
      break;
    }
  }
  assert(ret == RCL_RET_OK);

  printf("Received AddTwoInts Response with sum=%"PRId64"\n", client_resp.sum);

  example_interfaces__srv__AddTwoInts_Response__fini(&client_resp);

  ret = rcl_service_fini(&service, node_ptr);
  assert(ret == RCL_RET_OK);
  ret = rcl_client_fini(&client, node_ptr);
  assert(ret == RCL_RET_OK);

  // cleanup and shutdown
  ret = rcl_node_fini(node_ptr);
  assert(ret == RCL_RET_OK);
  ret = rcl_shutdown(context_ptr);
  assert(ret == RCL_RET_OK);
  ret = rcl_context_fini(context_ptr);
  assert(ret == RCL_RET_OK);
  free(node_ptr);
  free(context_ptr);


  ret = ret * 2; // squash assigned to but never used warning
}












 
