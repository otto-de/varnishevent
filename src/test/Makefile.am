AM_CPPFLAGS = @VARNISH_CFLAGS@ -I ${VARNISH_PKG_INCLUDE}

AM_TESTS_ENVIRONMENT = \
	PATH=@LIBVARNISHAPI_BINDIR@:$$PATH; export PATH;

TESTS = test_data test_strfTIM test_hdrtrie test_format test_writer \
	ncsa.sh vslarg.sh regress.sh

# artifacts of the regression test
CLEANFILES = test.log output.log

check_PROGRAMS = test_data test_strfTIM test_format test_writer test_hdrtrie

test_data_SOURCES = \
	minunit.h \
	test_data.c

test_data_LDADD = \
	../base64.$(OBJEXT) \
	../config.$(OBJEXT) \
	../log.$(OBJEXT) \
	../data.$(OBJEXT) \
	../format.$(OBJEXT) \
	../strfTIM.$(OBJEXT) \
	../hdrtrie.$(OBJEXT) \
	@VARNISH_LIBS@ @VARNISH_LIBVARNISH_LIB@ ${LIBM}

test_format_SOURCES = \
	minunit.h \
	test_format.c

test_format_LDADD = \
	../config.$(OBJEXT) \
	../strfTIM.$(OBJEXT) \
	../base64.$(OBJEXT) \
	../data.$(OBJEXT) \
	../hdrtrie.$(OBJEXT) \
	../format.$(OBJEXT) \
	@VARNISH_LIBS@ @VARNISH_LIBVARNISH_LIB@ ${LIBM}

test_writer_SOURCES = \
	minunit.h \
	test_writer.c

test_writer_LDADD = \
	../config.$(OBJEXT) \
	../format.$(OBJEXT) \
	../log.$(OBJEXT) \
	../data.$(OBJEXT) \
	../monitor.$(OBJEXT) \
	../base64.$(OBJEXT) \
	../spscq.$(OBJEXT) \
	../strfTIM.$(OBJEXT) \
	../hdrtrie.$(OBJEXT) \
	../writer.$(OBJEXT) \
	@VARNISH_LIBS@ @VARNISH_LIBVARNISH_LIB@ ${LIBM}

test_strfTIM_SOURCES = \
	minunit.h \
	../strfTIM.h \
	test_strfTIM.c

test_strfTIM_LDADD = \
	../strfTIM.$(OBJEXT) \
	@VARNISH_LIBS@  @VARNISH_LIBVARNISH_LIB@ ${LIBM}

test_hdrtrie_SOURCES = \
	minunit.h \
	../hdrtrie.h \
	test_hdrtrie.c

test_hdrtrie_LDADD = \
	../hdrtrie.$(OBJEXT) \
	@VARNISH_LIBS@  @VARNISH_LIBVARNISH_LIB@ ${LIBM}
