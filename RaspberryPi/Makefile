
EXE         := main.out
SRC         := $(wildcard *.cpp)
SRC         += $(wildcard src/*.cpp) $(wildcard src/io/*.cpp) $(wildcard lib/rtmidi/*.cpp) $(wildcard src/audio/*.cpp)
DIR         := .obj
SRCDIR      := .obj/src .obj/lib .obj/lib/rtmidi .obj/src/io .obj/src/hardware .obj/src/audio
OBJ         := $(SRC:%.cpp=$(DIR)/%.o)
DEP         := $(OBJ:.o=.d)

PLATFORM = $(shell uname)
ifneq ("$(wildcard /etc/os-release)","")
PLATFORM = $(shell . /etc/os-release && echo $$NAME)
endif

#override platform selection on RPi:
ifneq ("$(wildcard /opt/vc/include/bcm_host.h)","")
    PLATFORM = $(shell . /etc/os-release && echo $$PRETTY_NAME)
endif

ifeq ( $(PLATFORM),Raspbian GNU/Linux 8 (jessie) )
CPPFLAGS+= -DPLATFORM_RPI


else ifeq ($(PLATFORM),Raspbian GNU/Linux 9 (stretch))
CPPFLAGS+= -DPLATFORM_RPI

else ifeq ($(PLATFORM),Raspbian GNU/Linux 10 (buster))
CPPFLAGS+= -DPLATFORM_RPI

else ifeq ($(shell uname),Linux)
CPPFLAGS += -DPLATFORM_LINUX $(shell pkg-config --cflags glfw3 glu gl)
LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lpthread -ldl

else ifeq ($(PLATFORM),Darwin)
CXX = /usr/bin/clang++
ARCH = -arch x86_64
CFLAGS += $(ARCH) -DPLATFORM_OSX -stdlib=libc++ $(shell pkg-config --cflags glfw3)
INCLUDES += -I/System/Library/Frameworks/GLUI.framework
LDFLAGS += $(ARCH) -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit $(shell pkg-config --libs glfw3)
endif


CPPFLAGS+= -std=c++17 -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -D__LINUX_ALSA__ -DUSE_NEON

LDFLAGS+=-L$(SDKSTAGE)/opt/vc/lib/  -L$(SDKSTAGE)/opt/vc/src/hello_pi/libs/ilclient -L$(SDKSTAGE)/opt/vc/src/hello_pi/libs/vgfont

INCLUDES+=-I$(SDKSTAGE)/opt/vc/include/ -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux -I./ -I$(SDKSTAGE)/opt/vc/src/hello_pi/libs/ilclient -I$(SDKSTAGE)/opt/vc/src/hello_pi/libs/vgfont
INCLUDES+= -Isrc 

# NOTE: May need to export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/pi/VIDOS/lib/ndi/arm-rpi3-linux-gnueabihf/  to include NDI .so static libs
LDLIBS+= -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -lwiringPi -lm -lasound  $(wildcard lib/*.a) $(wildcard lib/*.so)

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(DIR)/%.o: %.cpp | $(DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -o $@ -c $<

$(DIR):
	@mkdir $@ $(SRCDIR)

-include $(DEP)

clean:
	$(RM) -r $(DIR) state.json
