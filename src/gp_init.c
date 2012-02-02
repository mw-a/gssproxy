/*
   GSS-PROXY

   Copyright (C) 2011 Red Hat, Inc.
   Copyright (C) 2011 Simo Sorce <simo.sorce@redhat.com>

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <syslog.h>
#include <signal.h>
#include "gp_proxy.h"

void init_server(bool daemonize)
{
    pid_t pid, sid;
    int ret;

    if (daemonize) {

        pid = fork();
        if (pid == -1) {
            /* fork error ? abort */
            exit(EXIT_FAILURE);
        }
        if (pid != 0) {
            /* ok kill the parent */
            exit(EXIT_SUCCESS);
        }

        sid = setsid();
        if (sid == -1) {
            /* setsid error ? abort */
            exit(EXIT_FAILURE);
        }
    }

    ret = chdir("/");
    if (ret == -1) {
        exit(EXIT_FAILURE);
    }

    /* Set strict umask by default */
    umask(0177);

    /* Set up neutral locale */
    setlocale(LC_ALL, "");

    openlog("gssproxy",
            LOG_CONS|LOG_NDELAY|LOG_NOWAIT|LOG_PERROR|LOG_PID,
            LOG_AUTHPRIV);
}

void fini_server(void)
{
    closelog();
}

static void break_loop(verto_ctx *vctx, verto_ev *ev)
{
    syslog(LOG_WARNING, "Exiting after receiving a signal");
    verto_break(vctx);
}

static void reload_conf(verto_ctx *vctx, verto_ev *ev)
{
    syslog(LOG_INFO, "Reloading configuration after receiving a signal");
    /* TODO */
}

verto_ctx *init_event_loop(void)
{
    verto_ctx *vctx;
    verto_ev *ev;

    vctx = verto_default(NULL,
                         VERTO_EV_TYPE_IO |
                         VERTO_EV_TYPE_SIGNAL |
                         VERTO_EV_TYPE_TIMEOUT);
    if (!vctx) {
        return NULL;
    }

    ev = verto_add_signal(vctx, VERTO_EV_FLAG_PERSIST, break_loop, SIGINT);
    if (!ev) {
        verto_free(vctx);
        return NULL;
    }
    ev = verto_add_signal(vctx, VERTO_EV_FLAG_PERSIST, break_loop, SIGTERM);
    if (!ev) {
        verto_free(vctx);
        return NULL;
    }
    ev = verto_add_signal(vctx, VERTO_EV_FLAG_PERSIST, break_loop, SIGQUIT);
    if (!ev) {
        verto_free(vctx);
        return NULL;
    }
    ev = verto_add_signal(vctx, VERTO_EV_FLAG_PERSIST, reload_conf, SIGHUP);
    if (!ev) {
        verto_free(vctx);
        return NULL;
    }
    ev = verto_add_signal(vctx, VERTO_EV_FLAG_PERSIST, VERTO_SIG_IGN, SIGPIPE);
    if (!ev) {
        verto_free(vctx);
        return NULL;
    }

    return vctx;
}
