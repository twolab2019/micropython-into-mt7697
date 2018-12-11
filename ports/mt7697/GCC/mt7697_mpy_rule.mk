include $(SOURCE_DIR)/.rule.mk
# ============================================================================
$(MY_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo Build... $$(basename $@)
	@echo Build... $@ >> $(BUILD_LOG)
	@if [ -e "$@" ]; then rm -f "$@"; fi
	@if [ -n "$(OVERRIDE_CFLAGS)" ]; then \
		echo $(CC) $(OVERRIDE_CFLAGS) $@ >> $(BUILD_LOG); \
		$(CC) $(OVERRIDE_CFLAGS) -c $< -o $@ 2>>$(ERR_LOG); \
	else \
		echo $(CC) $(CFLAGS) $@ >> $(BUILD_LOG); \
		$(CC) $(CFLAGS) -c $< -o $@ 2>>$(ERR_LOG); \
	fi; \
	if [ "$$?" != "0" ]; then \
		echo "Build... $$(basename $@) FAIL"; \
		echo "Build... $@ FAIL" >> $(BUILD_LOG); \
	else \
		echo "Build... $$(basename $@) PASS"; \
		echo "Build... $@ PASS" >> $(BUILD_LOG); \
	fi;

$(MY_BUILD_DIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; \
	export D_FILE="$@"; \
	export B_NAME=`echo $$D_FILE | sed 's/\.d//g'`; \
	if [ -n "$(OVERRIDE_CFLAGS)" ]; then \
		$(CC) -MM $(OVERRIDE_CFLAGS) $< > $@.$$$$; \
	else \
		$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	fi; \
	sed 's@\(.*\)\.o@'"$$B_NAME\.o $$B_NAME\.d"'@g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(MY_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo Build... $$(basename $@)
	@echo Build... $@ >> $(BUILD_LOG)
	@if [ -e "$@" ]; then rm -f "$@"; fi
	@if [ -n "$(OVERRIDE_CFLAGS)" ]; then \
		echo $(CXX) $(OVERRIDE_CFLAGS) $@ >> $(BUILD_LOG); \
		$(CXX) $(OVERRIDE_CFLAGS) -c $< -o $@ 2>>$(ERR_LOG); \
	else \
		echo $(CXX) $(CXXFLAGS) $@ >> $(BUILD_LOG); \
		$(CXX) $(CXXFLAGS) -c $< -o $@ 2>>$(ERR_LOG); \
	fi; \
	if [ "$$?" != "0" ]; then \
		echo "Build... $$(basename $@) FAIL"; \
		echo "Build... $@ FAIL" >> $(BUILD_LOG); \
	else \
		echo "Build... $$(basename $@) PASS"; \
		echo "Build... $@ PASS" >> $(BUILD_LOG); \
	fi;

$(MY_BUILD_DIR)/%.d: %.cpp
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; \
	export D_FILE="$@"; \
	export B_NAME=`echo $$D_FILE | sed 's/\.d//g'`; \
	if [ -n "$(OVERRIDE_CFLAGS)" ]; then \
		$(CXX) -MM $(OVERRIDE_CFLAGS) $< > $@.$$$$; \
	else \
		$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	fi; \
	sed 's@\(.*\)\.o@'"$$B_NAME\.o $$B_NAME\.d"'@g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(MY_BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo Build... $$(basename $@)
	@echo Build... $@ >> $(BUILD_LOG)
	@if [ -e "$@" ]; then rm -f "$@"; fi
	@if [ -n "$(OVERRIDE_CFLAGS)" ]; then \
		$(CC) $(OVERRIDE_CFLAGS) -c $< -o $@; \
	else \
		$(CC) $(CFLAGS) -c $< -o $@; \
	fi; \
	if [ "$$?" != "0" ]; then \
		echo "Build... $$(basename $@) FAIL"; \
		echo "Build... $@ FAIL" >> $(BUILD_LOG); \
	else \
		echo "Build... $$(basename $@) PASS"; \
		echo "Build... $@ PASS" >> $(BUILD_LOG); \
	fi;

