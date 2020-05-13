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

SRCS += common_fsm.c
SRCS += ring_buffer.c
SRCS += direct_buffer.c
SRCS += common_buffer.c
SRCS += async_task.c
SRCS += typedefs.c
SRCS += common_event.c
SRCS += common_queue.c
SRCS += flash_access.c
SRCS += prompt_flash.c
SRCS += log_print.c
SRCS += id3tag.c
SRCS += pcm_trans.c
SRCS += pcm_preprocess.c
SRCS += mp3_decoder.c
SRCS += aac_decoder.c
SRCS += common_player.c
SRCS += audio_file.c
SRCS += audio_player_process.c
SRCS += audio_message_queue.c
SRCS += audio_queue.c
SRCS += samplerate_convert.c
SRCS += http_download_process.c
SRCS += httpclient.c
SRCS += audio_test.c
SRCS += main.c

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
