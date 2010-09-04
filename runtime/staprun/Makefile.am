# Makefile.am --- automake input file for systemtap runtime tools

AM_CPPFLAGS = -D_GNU_SOURCE
AM_CFLAGS = -Wall -Werror -Wunused -W -Wformat=2 -Wno-format-nonliteral
AM_CPPFLAGS += -I$(srcdir)/../../includes
AM_CPPFLAGS += -DBINDIR='"$(bindir)"' -DSYSCONFDIR='"$(sysconfdir)"' -DPKGDATADIR='"${pkgdatadir}"' -DPKGLIBDIR='"$(pkglibexecdir)"'

AM_CFLAGS += @PIECFLAGS@
AM_LDFLAGS = @PIELDFLAGS@

bin_PROGRAMS = staprun stap-merge
pkglibexec_PROGRAMS = stapio

staprun_SOURCES = staprun.c staprun_funcs.c ctl.c common.c
staprun_CFLAGS = $(AM_CFLAGS) -DSINGLE_THREADED
staprun_LDADD =

if HAVE_NSS
staprun_SOURCES += modverify.c ../../nsscommon.c
staprun_CFLAGS += $(nss_CFLAGS)
staprun_LDADD += $(nss_LIBS)
endif

stapio_SOURCES = stapio.c mainloop.c common.c ctl.c relay.c relay_old.c
stapio_LDADD = -lpthread

man_MANS = staprun.8

stap_merge_SOURCES = stap_merge.c
stap_merge_CFLAGS = $(AM_CFLAGS)
stap_merge_LDFLAGS = $(AM_LDFLAGS)
stap_merge_LDADD =


# Why the "id -u" condition?  This way, an unprivileged user can run
# make install, and have "sudo stap ...." or "sudo staprun ...." work later.
install-exec-hook:
	[ `id -u` -ne 0 ] || chmod 04111 "$(DESTDIR)$(bindir)/staprun"