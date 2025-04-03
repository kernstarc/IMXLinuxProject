ifneq ($(svn_ver),)
EXTRA_CFLAGS += -DSVN_VER="\"$(svn_ver) \""
else
EXTRA_CFLAGS += -DSVN_VER=""
endif

ifneq ($(git_ver),)
EXTRA_CFLAGS += -DGIT_VER="\"commit:$(git_ver) \""
else
EXTRA_CFLAGS += -DGIT_VER=""
endif


obj-y := chrdevbase.o
