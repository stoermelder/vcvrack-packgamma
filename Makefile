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

$(gamma):
	cd dep/Gamma && $(MAKE) NO_AUDIO_IO=1 NO_SOUNDFILE=1
	cd dep/Gamma && $(MAKE) install NO_AUDIO_IO=1 NO_SOUNDFILE=1


include $(RACK_DIR)/plugin.mk


win-dist: all
	rm -rf dist
	mkdir -p dist/$(SLUG)
	@# Strip and copy plugin binary
	cp $(TARGET) dist/$(SLUG)/
ifdef ARCH_MAC
	$(STRIP) -S dist/$(SLUG)/$(TARGET)
else
	$(STRIP) -s dist/$(SLUG)/$(TARGET)
endif
	@# Copy distributables
	cp -R $(DISTRIBUTABLES) dist/$(SLUG)/
	@# Create ZIP package
	echo "cd dist && 7z.exe a $(SLUG)-$(VERSION)-$(ARCH).zip -r $(SLUG)"
	cd dist && 7z.exe a $(SLUG)-$(VERSION)-$(ARCH).zip -r $(SLUG)