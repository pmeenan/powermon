#include "PowerStats.h"

#define MONGOOSE_NO_FILESYSTEM
#include "lib/mongoose/mongoose.h"

bool must_exit = false;
PowerStats * power_stats = NULL;

/*-----------------------------------------------------------------------------
  Handle events from the web server
-----------------------------------------------------------------------------*/
static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
  switch (ev) {
    case MG_AUTH: return MG_TRUE;
    case MG_REQUEST: {
        std::string response;
        std::string uri = conn->uri;
        if (uri == "/start") {
          if (power_stats)
            response = power_stats->Start();
        } else if (uri == "/measure") {
            response = power_stats->Measure(conn->query_string);
        } else {
          response = conn->uri + std::string(" - Unknown request.  Valid requests are /start and /measure");
        }
        mg_send_data(conn, response.c_str(), response.length());
        return MG_TRUE;
      }
    default: return MG_FALSE;
  }
}

#ifdef WIN32
BOOL CtrlHandler(DWORD fdwCtrlType) {
  printf("Exiting...\n");
  must_exit = true;
  return TRUE;
}
#endif WIN32

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
int main(void) {
#ifdef WIN32
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#endif WIN32

  struct mg_server *server;
  power_stats = new PowerStats();

  // Create and configure the server
  server = mg_create_server(NULL, ev_handler);
  mg_set_option(server, "listening_port", "8765");

  // Serve request. Hit Ctrl-C to terminate the program
  printf("Starting on port %s\n", mg_get_option(server, "listening_port"));
  while (!must_exit) {
    mg_poll_server(server, 3000);
  }

  // Cleanup, and free server instance
  mg_destroy_server(&server);
  delete power_stats;

  return 0;
}