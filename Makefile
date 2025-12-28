RACK_DIR ?= ../..

FLAGS += \
	-Idep/Gamma

SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin is automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res

gamma := dep/Gamma/build/lib/libGamma.a

# Static libs
OBJECTS += dep/Gamma/build/lib/libGamma.a

# Dependencies
DEP_LOCAL := dep
DEPS += $(gamma)


# Ensure the Gamma build depends on the Makefile.user when cross-compiling
ifneq ($(strip $(GAMMA_WINDOWS)),)
GAMMA_MAKEFILE_USER := dep/Gamma/Makefile.user

$(GAMMA_MAKEFILE_USER):
	mkdir -p dep/Gamma
	@printf '%s\n' '# Force Windows platform for Gamma build and avoid adding host /usr includes' 'PLATFORM := windows' > $@

$(gamma): $(GAMMA_MAKEFILE_USER)
endif

$(gamma):
	mkdir -p dep/Gamma
	git submodule update --init --recursive dep/Gamma
	cd dep/Gamma && $(MAKE) NO_AUDIO_IO=1 NO_SOUNDFILE=1


include $(RACK_DIR)/plugin.mk