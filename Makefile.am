lib_LTLIBRARIES = librabbitmq.la

AM_CFLAGS = -I$(srcdir)/$(PLATFORM_DIR) -DNDEBUG
librabbitmq_la_SOURCES = amqp_mem.c amqp_utils.c amqp_logging.c amqp_table.c amqp_connection.c amqp_socket.c amqp_debug.c amqp_api.c $(PLATFORM_DIR)/socket.c
librabbitmq_la_LDFLAGS = -no-undefined -DNDEBUG
librabbitmq_la_LIBADD = $(EXTRA_LIBS)
nodist_librabbitmq_la_SOURCES = amqp_framing.c
include_HEADERS = amqp_framing.h amqp.h
noinst_HEADERS = amqp_private.h $(PLATFORM_DIR)/socket.h
BUILT_SOURCES = amqp_framing.h amqp_framing.c
CLEANFILES = amqp_framing.h amqp_framing.c
EXTRA_DIST = \
	codegen.py \
	unix/socket.c unix/socket.h \
	windows/socket.c windows/socket.h \
	windows/build/librabbitmq/librabbitmq.aps \
	windows/build/librabbitmq/librabbitmq.rc \
	windows/build/librabbitmq/librabbitmq.sln \
	windows/build/librabbitmq/librabbitmq.suo \
	windows/build/librabbitmq/librabbitmq.vcxproj \
	windows/build/librabbitmq/librabbitmq.vcxproj.filters \
	windows/build/librabbitmq/librabbitmq.vcxproj.users \
	windows/build/librabbitmq/resource.h

CODEGEN_PY=$(srcdir)/codegen.py

amqp_framing.h: $(AMQP_SPEC_JSON_PATH) $(CODEGEN_PY)
	PYTHONPATH=$(AMQP_CODEGEN_DIR) $(PYTHON) $(CODEGEN_PY) header $< $@

amqp_framing.c: $(AMQP_SPEC_JSON_PATH) $(CODEGEN_PY)
	PYTHONPATH=$(AMQP_CODEGEN_DIR) $(PYTHON) $(CODEGEN_PY) body $< $@
