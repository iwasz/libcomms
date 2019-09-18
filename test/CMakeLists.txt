CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET (CMAKE_VERBOSE_MAKEFILE OFF)

# hardcoded!
#SET (CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS "-std=gnu99 -Wall -pthread" CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall -pthread" CACHE INTERNAL "cxx compiler flags")
ADD_DEFINITIONS ("-DUNIT_TEST")

PROJECT (libcommstest)
ENABLE_TESTING ()
INCLUDE_DIRECTORIES("../src/")
INCLUDE_DIRECTORIES("../test/")

SET (ETL_INCLUDE "../../etl/include")
INCLUDE_DIRECTORIES("${ETL_INCLUDE}")

SET (LIB_MICRO_ROOT "../../libmicro")
INCLUDE_DIRECTORIES ("${LIB_MICRO_ROOT}/src")

SET (LIB_COMMS_ROOT "../../libcomms")
INCLUDE_DIRECTORIES ("${LIB_COMMS_ROOT}/src")

SET (LIB_STATE_MACHINE_ROOT "../../libstatemachine")
INCLUDE_DIRECTORIES ("${LIB_STATE_MACHINE_ROOT}/src")

SET (MQTT_PACKET_SRC "../../paho.mqtt.embedded-c/MQTTPacket/src")
INCLUDE_DIRECTORIES(${MQTT_PACKET_SRC})

INCLUDE_DIRECTORIES("../../ium/src")

add_definitions ("-DX86_TEST")

ADD_EXECUTABLE(libmicrotest
    "etl_profile.h"
    "00CatchInit.cc"
#    "01Mc60Test.cc"
    "01QirdiTest.cc"

    "../src/mc60Modem/QirdiParseAction.h"

#    "../../ium/src/MqttClient.cc"
#    "../../ium/src/MqttClient.h"

    "${LIB_MICRO_ROOT}/src/ErrorHandler.h"
    "${LIB_MICRO_ROOT}/src/ErrorHandler.c"
#    "${LIB_MICRO_ROOT}/src/Timer.h"
#    "${LIB_MICRO_ROOT}/src/TimerUnit.cc"
#    "${LIB_MICRO_ROOT}/src/Gpio.h"
#    "${LIB_MICRO_ROOT}/src/GpioX86T.h"
#    "${LIB_MICRO_ROOT}/src/GpioX86T.cc"
#    "${LIB_MICRO_ROOT}/src/Usart.h"
#    "${LIB_MICRO_ROOT}/src/UsartX86T.h"
#    "${LIB_MICRO_ROOT}/src/UsartX86T.cc"
#    "${LIB_MICRO_ROOT}/src/Hal.h"
#    "${LIB_MICRO_ROOT}/src/Debug.cc"
#    "${LIB_MICRO_ROOT}/src/Debug.h"
#    "${LIB_MICRO_ROOT}/src/x86-test/Engine.cc"
#    "${LIB_MICRO_ROOT}/src/x86-test/Engine.h"
#    "${LIB_MICRO_ROOT}/src/debug.h"
#    "${LIB_MICRO_ROOT}/src/collection/Queue.h"
#    "${LIB_MICRO_ROOT}/src/collection/CircularBuffer.h"
#    "${LIB_MICRO_ROOT}/src/collection/CircularBuffer.cc"
#    "${LIB_MICRO_ROOT}/src/character/ICharacterSink.h"
#    "${LIB_MICRO_ROOT}/src/character/BufferedCharacterSink.h"
#    "${LIB_MICRO_ROOT}/src/character/LineSink.h"
#    "${LIB_MICRO_ROOT}/src/character/FixedLineSink.h"

#    "${LIB_COMMS_ROOT}/src/ICommunicationInterface.h"
#    "${LIB_COMMS_ROOT}/src/AbstractModem.h"
#    "${LIB_COMMS_ROOT}/src/BinaryEvent.h"
#    "${LIB_COMMS_ROOT}/src/UsartAction.h"
#    "${LIB_COMMS_ROOT}/src/mc60Modem/Mc60Modem.cc"
#    "${LIB_COMMS_ROOT}/src/mc60Modem/Mc60Modem.h"
#    "${LIB_COMMS_ROOT}/src/mc60Modem/SendNetworkAction.h"
#    "${LIB_COMMS_ROOT}/src/mc60Modem/SendNetworkAction.cc"
#    "${LIB_COMMS_ROOT}/src/mc60Modem/QueryAckAction.h"
#    "${LIB_COMMS_ROOT}/src/mc60Modem/QueryRecvAction.h"
#    "${LIB_COMMS_ROOT}/src/modem/StatusPinCondition.h"
#    "${LIB_COMMS_ROOT}/src/modem/StatusPinCondition.cc"
#    "${LIB_COMMS_ROOT}/src/modem/PwrKeyAction.h"
#    "${LIB_COMMS_ROOT}/src/modem/PwrKeyAction.cc"
#    "${LIB_COMMS_ROOT}/src/modem/GsmCommandAction.h"
#    "${LIB_COMMS_ROOT}/src/modem/GsmCommandAction.cc"

#    "${LIB_STATE_MACHINE_ROOT}/src/Action.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/DelayAction.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/State.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/StateMachine.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/TimeCounter.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/Transition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/Condition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/StringCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/TimePassedCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/BeginsWithCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/LikeCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/NotCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/BoolCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/BoolAction.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/IntegerAction.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/IntegerCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/OrCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/AndCondition.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/StateChangeAction.h"
#    "${LIB_STATE_MACHINE_ROOT}/src/StateCondition.h"

#    "${MQTT_PACKET_SRC}/MQTTConnectClient.c"
#    "${MQTT_PACKET_SRC}/MQTTConnect.h"
#    "${MQTT_PACKET_SRC}/MQTTConnectServer.c"
#    "${MQTT_PACKET_SRC}/MQTTDeserializePublish.c"
#    "${MQTT_PACKET_SRC}/MQTTFormat.c"
#    "${MQTT_PACKET_SRC}/MQTTFormat.h"
#    "${MQTT_PACKET_SRC}/MQTTPacket.c"
#    "${MQTT_PACKET_SRC}/MQTTPacket.h"
#    "${MQTT_PACKET_SRC}/MQTTPublish.h"
#    "${MQTT_PACKET_SRC}/MQTTSerializePublish.c"
#    "${MQTT_PACKET_SRC}/MQTTSubscribeClient.c"
#    "${MQTT_PACKET_SRC}/MQTTSubscribe.h"
#    "${MQTT_PACKET_SRC}/MQTTSubscribeServer.c"
#    "${MQTT_PACKET_SRC}/MQTTUnsubscribeClient.c"
#    "${MQTT_PACKET_SRC}/MQTTUnsubscribe.h"
#    "${MQTT_PACKET_SRC}/MQTTUnsubscribeServer.c"
#    "${MQTT_PACKET_SRC}/StackTrace.h"
)

ADD_TEST (libcommstest libcommstest)
