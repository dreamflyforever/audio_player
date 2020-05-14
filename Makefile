TARGET  := emplayer
SRC_DIR := src
OBJ_DIR := objs
BIN_DIR := bin
CC      := gcc

CFLAGS += -Wall -g -DDEF_AAC_DECODER_ENABLE -DUSE_SMALLFT
CFLAGS += -DFLOATING_POINT -DDEF_LINUX_PLATFORM 
CFLAGS += -DES8388_ENABLE -DSTEREO_ENABLE
#CFLAGS += -DFIXED_POINT

INCS += -I.
INCS += -Iapp
INCS += -Isrc
INCS += -Ios
INCS += -Iprompt
INCS += -Iutils

SRCS += ./app/main.c
SRCS += ./app/audio_test.c
SRCS += ./src/pcm_preprocess.c
SRCS += ./src/audio_player_process.c
SRCS += ./src/common_player.c
SRCS += ./src/common_queue.c
SRCS += ./src/pcm_trans.c
SRCS += ./src/aac_decoder.c
SRCS += ./src/common_event.c
SRCS += ./src/audio_file.c
SRCS += ./src/id3tag.c
SRCS += ./src/samplerate_convert.c
SRCS += ./src/audio_message_queue.c
SRCS += ./src/mp3_decoder.c
SRCS += ./src/audio_queue.c
SRCS += ./os/typedefs.c
SRCS += ./prompt/prompt_flash.c
SRCS += ./prompt/flash_access.c
SRCS += ./utils/httpclient.c
SRCS += ./utils/direct_buffer.c
SRCS += ./utils/common_buffer.c
SRCS += ./utils/http_download_process.c
SRCS += ./utils/common_fsm.c
SRCS += ./utils/async_task.c
SRCS += ./utils/ring_buffer.c
SRCS += ./utils/log_print.c

DATS += prompt_data_1.dat
DATS += prompt_data_2.dat
DATS += prompt_data_3.dat
DATS += prompt_data_4.dat

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
OBJS += $(patsubst %.dat,$(OBJ_DIR)/%.o,$(DATS))
DEPS += $(patsubst %.c,$(OBJ_DIR)/%.d,$(SRCS))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo build $<
	@mkdir -p $(dir $@)
	@$(CC) -c $(CFLAGS) $< -o $@ $(INCS)
	
$(OBJ_DIR)/%.o: %.c
	@echo build $<
	@mkdir -p $(dir $@)
	@$(CC) -c $(CFLAGS) $< -o $@ $(INCS)
	
$(OBJ_DIR)/%.o: %.dat
	@echo build $<
	@mkdir -p $(dir $@)
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; var=`echo $@ $@ | sed 's/\.d/\.o/'`; $(CC) $(CFLAGS) $(INCS) -MM $^ | sed 's,\(.*\)\.o[ :]*,'"$$var"' : ,g' > $@;

$(OBJ_DIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; var=`echo $@ $@ | sed 's/\.d/\.o/'`; $(CC) $(CFLAGS) $(INCS) -MM $^ | sed 's,\(.*\)\.o[ :]*,'"$$var"' : ,g' > $@;

all: $(OBJS)
	@mkdir -p $(BIN_DIR)
	@$(CC) $(CFLAGS) -o $(BIN_DIR)/$(TARGET) $(OBJS) -lasound -lpthread -lmad -lfaad -lmp4ff -lm -lsamplerate
	@echo build done

.PHONY: all clean

clean:
	@rm -rf objs

-include $(DEPS)
