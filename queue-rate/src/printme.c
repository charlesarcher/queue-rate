#include <stdbool.h>
#include <hwloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


int urandom_init(){
        int urandom_fd = open("/dev/urandom", O_RDONLY);
        if(urandom_fd == -1){
                int errsv = urandom_fd;
                printf("Error opening [/dev/urandom]: %i\n", errsv);
                exit(1);
        }
        return urandom_fd;
}

unsigned long urandom(int urandom_fd){
        unsigned long buf_impl;
        unsigned long *buf = &buf_impl;
        read(urandom_fd, buf, sizeof(long));
        return buf_impl;
}

/* ************************************************* */
static char *bitmap2rangestr(int bitmap)
{
    size_t i;
    int range_start, range_end;
    bool first, isset;
    char tmp[BUFSIZ];
    const int stmp = sizeof(tmp) - 1;
    static char ret[BUFSIZ];

    memset(ret, 0, sizeof(ret));

    first = true;
    range_start = -999;
    for (i = 0; i < sizeof(int) * 8; ++i) {
        isset = (bitmap & (1 << i));

        /* Do we have a running range? */
        if (range_start >= 0) {
            if (isset) {
                continue;
            } else {
                /* A range just ended; output it */
                if (!first) {
                    strncat(ret, ",", sizeof(ret) - strlen(ret) - 1);
                } else {
                    first = false;
                }

                range_end = i - 1;
                if (range_start == range_end) {
                    snprintf(tmp, stmp, "%d", range_start);
                } else {
                    snprintf(tmp, stmp, "%d-%d", range_start, range_end);
                }
                strncat(ret, tmp, sizeof(ret) - strlen(ret) - 1);

                range_start = -999;
            }
        }

        /* No running range */
        else {
            if (isset) {
                range_start = i;
            }
        }
    }

    /* If we ended the bitmap with a range open, output it */
    if (range_start >= 0) {
        if (!first) {
            strncat(ret, ",", sizeof(ret) - strlen(ret) - 1);
            first = false;
        }

        range_end = i - 1;
        if (range_start == range_end) {
            snprintf(tmp, stmp, "%d", range_start);
        } else {
            snprintf(tmp, stmp, "%d-%d", range_start, range_end);
        }
        strncat(ret, tmp, sizeof(ret) - strlen(ret) - 1);
    }

    return ret;
}


int printme(char *instr) {
  bool first;
  hwloc_topology_t topology;
  int nbcores,nbsockets,socket_index,pu_index,core_index;
  hwloc_obj_t socket, core, pu;
  int **data;
  hwloc_cpuset_t cpuset;

  hwloc_topology_init(&topology);  // initialization
  hwloc_topology_load(topology);   // actual detection

  cpuset = hwloc_bitmap_alloc();
  if (hwloc_get_cpubind(topology,
                        cpuset,
                        HWLOC_CPUBIND_THREAD) < 0)
    {
      fprintf(stdout, "Not Bound\n");
      return 0;
    }

  nbsockets = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_SOCKET);
  nbcores   = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
/*   printf("%d cores %d sockets\n", nbcores, nbsockets);*/

  data = (int**)malloc(nbsockets * sizeof(int *));
  if (NULL == data) {
    fprintf(stderr, "Error in allocation\n");
    return 1;
  }
  data[0] = (int*)calloc(nbsockets * nbcores, sizeof(int));
  if (NULL == data[0]) {
    fprintf(stderr, "Error in allocation\n");
    free(data);
    return 1;
  }
  for (socket_index = 1; socket_index < nbsockets; ++socket_index) {
    data[socket_index] = data[socket_index - 1] + nbcores;
  }

  for (pu_index = 0,
         pu = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                  cpuset, HWLOC_OBJ_PU,
                                                  pu_index);
       NULL != pu;
       pu = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                cpuset, HWLOC_OBJ_PU,
                                                ++pu_index)) {
    /* Go upward and find the core this PU belongs to */
    core = pu;
    while (NULL != core && core->type != HWLOC_OBJ_CORE) {
      core = core->parent;
    }
    core_index = 0;
    if (NULL != core) {
      core_index = core->logical_index;
    }

    /* Go upward and find the socket this PU belongs to */
    socket = pu;
    while (NULL != socket && socket->type != HWLOC_OBJ_SOCKET) {
      socket = socket->parent;
    }
    socket_index = 0;
    if (NULL != socket) {
      socket_index = socket->logical_index;
    }

    /* Save this socket/core/pu combo.  LAZY: Assuming that we
       won't have more PU's per core than (sizeof(int)*8). */
    data[socket_index][core_index] |= (1 << pu->sibling_rank);
  }
  int **map = data;
  char str1[1024],str2[1024];
  char tmp1[1024],tmp2[1024];
  int len = 1024;
  const int stmp = sizeof(tmp1) - 1;
  str1[0] = tmp1[stmp] = '\0';
  str2[0] = tmp2[stmp] = '\0';
  first = true;
  for (socket_index = 0; socket_index < nbsockets; ++socket_index) {
    for (core_index = 0; core_index < nbcores; ++core_index) {
      if (map[socket_index][core_index] > 0) {
        if (!first) {
          strncat(str1, ", ", len - strlen(str1));
        }
        first = false;

        snprintf(tmp1, stmp, "socket %d[core %d[hwt %s]]",
                 socket_index, core_index,
                 bitmap2rangestr(map[socket_index][core_index]));
        strncat(str1, tmp1, len - strlen(str1));
      }
    }
  }
  free(map[0]);
  free(map);

  /* Iterate over all existing sockets */
  for (socket = hwloc_get_obj_by_type(topology, HWLOC_OBJ_SOCKET, 0);
       NULL != socket;
       socket = socket->next_cousin) {
    strncat(str2, "[", len - strlen(str2));

    /* Iterate over all existing cores in this socket */
    core_index = 0;
    for (core = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                    socket->cpuset,
                                                    HWLOC_OBJ_CORE, core_index);
         NULL != core;
         core = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                    socket->cpuset,
                                                    HWLOC_OBJ_CORE, ++core_index)) {
      if (core_index > 0) {
        strncat(str2, "/", len - strlen(str2));
      }

      /* Iterate over all existing PUs in this core */
      pu_index = 0;
      for (pu = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                    core->cpuset,
                                                    HWLOC_OBJ_PU, pu_index);
           NULL != pu;
           pu = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                    core->cpuset,
                                                    HWLOC_OBJ_PU, ++pu_index)) {

        /* Is this PU in the cpuset? */
        if (hwloc_bitmap_isset(cpuset, pu->os_index)) {
          strncat(str2, "B", len - strlen(str2));
        } else {
          strncat(str2, ".", len - strlen(str2));
        }
      }
    }
    strncat(str2, "]", len - strlen(str2));
  }

  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
/*fprintf(stdout, "%s:%s\n", str1, str2);*/
  fprintf(stdout, "%s: %s %s\n", hostname,instr, str2);

  hwloc_topology_destroy(topology);

  return 0;
}
/* ************************************************ */

