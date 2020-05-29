#
# Copyright Broadcom corporation (2017)
#

OBJ_Y_SRCS = $(filter-out %/, $(obj-y))
OBJ_Y_SUBDIRS = $(filter %/, $(obj-y))

SUBDIRS += $(OBJ_Y_SUBDIRS) $(SUBDIRS-y) $(SUBDIRS-1) $(subdirs-y) $(subdirs-1)

# Targets for subdir makes
ifneq ($(SUBDIRS),"")
    all: subdirs
endif

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

# Function to add subdir recipe
define add_subdir_recipe
$(1):
	$(Q)$(MAKE) $(2) -C $1
endef

$(foreach S,$(sort $(SUBDIRS)), $(eval $(call add_subdir_recipe,$(S),$(MAKEOPTS))))

# Targets and flags for compilation units .[csS]
CWD = $(shell pwd)

OBJECTS = $(addprefix $(subst $(MPOS_BASE),$(BUILDDIR),$(CWD))/, $(OBJS) $(OBJ_Y_SRCS))
DEPS = $(filter-out %.a,$(OBJECTS:%.o=%.d))

ifneq ($(OBJECTS),"")
all: $(OBJECTS)
endif

$(BUILDDIR)/%.o: $(MPOS_BASE)/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo [Compiling ... $(notdir $<)]
	$(Q)$(CC) $(CFLAGS) -c -MD -o $@ $<

$(BUILDDIR)/%.o: $(MPOS_BASE)/%.s
	$(Q)mkdir -p $(dir $@)
	$(Q)echo [Compiling ... $(notdir $<)]
	$(Q)$(CC) $(CFLAGS) -c -MD -o $@ $<

$(BUILDDIR)/%.o: $(MPOS_BASE)/%.S
	$(Q)mkdir -p $(dir $@)
	$(Q)echo [Compiling ... $(notdir $<)]
	$(Q)$(CC) $(CFLAGS) -c -MD -o $@ $<

$(BUILDDIR)/%.a: $(MPOS_BASE)/%.a
	$(Q)mkdir -p $(dir $@)
	$(Q)echo [Copying ... $(notdir $<)]
	$(Q)$(CP) -f $< $@

-include $(DEPS)
