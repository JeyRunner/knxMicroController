cmake_minimum_required(VERSION 3.0)
SET(CMAKE_SYSTEM_NAME Generic)


#============================================================================================
project(knxMu)

set (       SRC_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/src/)
file( GLOB  SRC_FILES ${SRC_DIR}*.cpp ${SRC_DIR}*.h  ${SRC_DIR}log/*.cpp ${SRC_DIR}log/*.h)

# microcontroller settings
SET(DEVICE "atmega32")
SET(FREQ "8000000") # 8 MHz

# programmer settings
SET(PROGRAMMER avrispmkII)


#============================================================================================

SET(CMAKE_C_COMPILER avr-gcc)
SET(CMAKE_CXX_COMPILER avr-g++)

SET(CMAKE_C_FLAGS  "-Os -mmcu=${DEVICE} -DF_CPU=${FREQ}UL -std=gnu99 -Wl,--gc-sections")
SET(CMAKE_CXX_FLAGS "-Os -mmcu=${DEVICE} -DF_CPU=${FREQ}UL -Wl,--gc-sections")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

#============================================================================================

include_directories( ${SRC_DIR}log/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_executable(${PROJECT_NAME} ${SRC_FILES} src/log/new.h)


#============================================================================================
#== programm to hex
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD COMMAND avr-objcopy -O ihex ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex)
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD COMMAND avr-size ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME} --mcu=${DEVICE} --format=avr)

set(HEX_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex)
set(EEP_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.eep)

add_custom_target(
        upload_${TARGET}
        ${AVRDUDE} -p ${DEVICE} -c ${PROGRAMMER} ${AVRDUDE_OPTIONS}
        -U flash:w:${HEX_FILE}
        -U eeprom:w:${EEP_FILE}
        -P usb -b 115200 # XXX
        DEPENDS ${HEX_FILE} ${EEP_FILE}
        COMMENT "Uploading ${HEX_FILE} to ${DEVICE} using programmer ${AVR_PROGRAMMER}"
)

#============================================================================================
#== programm flash
#ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD COMMAND avrdude -p atmega16 -c avrisp2 -U flash:w:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex)