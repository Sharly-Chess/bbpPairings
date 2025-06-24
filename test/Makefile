optional_cxxflags = \
	-std=c++20 \
	-ftabstop=2 \
	$(if $(filter-out mac,$(target_os)),-Werror) \
	-Wfatal-errors \
	-pedantic \
	-pedantic-errors \
	-Wall \
	-Walloca \
	-Wcast-qual \
	-Wctor-dtor-privacy \
	-Wdisabled-optimization \
	-Wdouble-promotion \
	-Wextra \
	-Wextra-semi \
	-Wformat=2 \
	-Winvalid-pch \
	-Wmismatched-tags \
	-Wmissing-braces \
	-Wmissing-declarations \
	-Wmissing-include-dirs \
	-Wnull-dereference \
	-Woverloaded-virtual \
	-Wpacked \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wsign-promo \
	-Wstrict-overflow=4 \
	-Wsuggest-override \
	-Wswitch-default \
	-Wundef \
	-Wunknown-pragmas \
	-Wunused-macros \
	-Wvla \
	-Wzero-as-null-pointer-constant \
	-Wno-free-nonheap-object \
	-Wno-overflow \
	-Wno-sign-compare

ifeq ($(shell uname -o),Msys)
	optional_cxxflags += -static
endif

# Use clang or clang++ for macOS
ifeq ($(shell uname),Darwin)
	CXX = clang++
else
	CXX = g++
endif

ifeq ($(CXX),g++)
	optional_cxxflags += \
		-Wabi=0 \
		-Waligned-new=all \
		-Warray-bounds=2 \
		-Wattribute-alias=2 \
		-Wcast-align=strict \
		-Wcatch-value=3 \
		-Wconditionally-supported \
		-Wduplicated-cond \
		-Wformat-overflow=2 \
		-Wformat-signedness \
		-Wformat-truncation=2 \
		-Wimplicit-fallthrough=3 \
		-Wlogical-op \
		-Wmultiple-inheritance \
		-Wnormalized=nfkc \
		-Wplacement-new=2 \
		-Wredundant-tags \
		-Wshadow=local \
		-Wstrict-null-sentinel \
		-Wstringop-overflow=4 \
		-Wsuggest-attribute=cold \
		-Wsuggest-attribute=format \
		-Wsuggest-attribute=malloc \
		-Wsuggest-attribute=noreturn \
		-Wsuggest-final-methods \
		-Wsuggest-final-types \
		-Wsync-nand \
		-Wtrampolines \
		-Wunsafe-loop-optimizations \
		-Wvector-operation-performance \
		-Wvirtual-inheritance \
		-Wno-maybe-uninitialized
endif

CXXFLAGS = $(optional_cxxflags)

TESTS = tests

TEST_SOURCES = $(shell find $(TESTS) -name "*.cpp")
TEST_IDS = $(patsubst $(TESTS)/%.cpp, %, $(TEST_SOURCES))

.DELETE_ON_ERROR:

all: bbpPairingsTests.exe
.PHONY: all

test-includes.h: $(TESTS)
	echo > $@
	$(foreach \
		test_id, \
		$(TEST_IDS), \
		echo "#define TEST_ID $(test_id)" >> $@; \
			echo "#include <tests/$(test_id).cpp>" >> $@; \
			echo "#undef TEST_ID" >> $@;)
	echo >> $@
	echo "int runTests(const testing::Context &context)" >> $@
	echo { >> $@
	echo "  BEFORE_RUNNING_TESTS" >> $@
	$(foreach test_id, $(TEST_IDS), echo "  RUN_TEST($(test_id))" >> $@;)
	echo "  AFTER_RUNNING_TESTS" >> $@
	echo } >> $@

bbpPairingsTests.exe: test-includes.h main.cpp
	$(CXX) -o $@ -I. -MMD -MP main.cpp $(CXXFLAGS)

-include bbpPairingsTests.d

run: bbpPairingsTests.exe
	./bbpPairingsTests.exe ../bbpPairings.exe $(TESTS)
.PHONY: run

clean:
	$(RM) -r $(TESTS)/*.output
	$(RM) -r bbpPairingsTests.*
	$(RM) -r test-includes.h
.PHONY: clean
