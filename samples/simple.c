// comment line
void dump(TxNode *n, int level);

/*
this is a comment block test's
*/

#include "textmate.h"

int main(int argc, char **argv) {
  for (int i = 0; i < 200; i++) {
    printf("%d %d\n", i, i);
    printf("%d %d\n",
      i,
      i);
  }

  printf("%d %d\n", i, i);
  return 0;
}

int main() {
  /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
  setenv("WAYLAND_DISPLAY", socket, true);
  if (startup_cmd) {
    if (fork() == 0) {
      execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *)NULL);
    }
  }
  /* Run the Wayland event loop. This does not return until you exit the
   * compositor. Starting the backend rigged up all of the necessary event
   * loop configuration to listen to libinput events, DRM events, generate
   * frame events at the refresh rate, and so on. */

  // wlr_seat_create(server.wl_display, "seat0");

  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=",
      socket);
  wl_display_run(server.wl_display);
  return 0;
}