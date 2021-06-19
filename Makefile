CC=gcc
CFLAGS=-std=gnu89 -Wall -Wextra -Wno-unused-parameter -Wno-overlength-strings -Wno-long-long -Wno-format -pedantic -I src
CFLAGS_D=$(CFLAGS) -O0 -g2
CFLAGS_R=$(CFLAGS) -O2 -DNDEBUG
LDFLAGS_D=
LDFLAGS_R=

INCDIR=src
SRCDIR=src
OBJDIR=obj
TMPDIR_D=debug/tmp
TMPDIR_R=release/tmp
BINDIR_D=debug/bin
BINDIR_R=release/bin
OBJDIR_D=debug/obj
OBJDIR_R=release/obj

OBJNAMES := chararray common context generate main match node parse util

OBJS_D := $(foreach obj, $(OBJNAMES), $(OBJDIR_D)/$(obj).o)
OBJS_R := $(foreach obj, $(OBJNAMES), $(OBJDIR_R)/$(obj).o)

OBJS := $(OBJS_D) $(OBJS_R)

INCS=$(INCDIR)/*.h

BINS= \
  $(BINDIR_D)/packzz \
  $(BINDIR_R)/packzz \

.PHONY: all check clean

.SECONDARY: $(GENERATEDSRCS)

%.c: $(INCS)

$(OBJDIR_D)/%.o: $(SRCDIR)/%.c
	mkdir -p $(dir $@) && $(CC) $(CFLAGS_D) -c $< -o $@
$(OBJDIR_R)/%.o: $(SRCDIR)/%.c
	mkdir -p $(dir $@) && $(CC) $(CFLAGS_R) -c $< -o $@

all: $(BINS)

$(BINDIR_D)/packzz: $(OBJS_D)
	mkdir -p $(dir $@) && $(CC) $(CFLAGS_D) -o $@ $^ $(LDFLAGS_D)

$(BINDIR_R)/packzz: $(OBJS_R)
	mkdir -p $(dir $@) && $(CC) $(CFLAGS_R) -o $@ $^ $(LDFLAGS_R)

clean:
	rm -f $(BINS) $(OBJS)
