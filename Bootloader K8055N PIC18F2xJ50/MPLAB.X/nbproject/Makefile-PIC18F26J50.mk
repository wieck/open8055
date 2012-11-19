#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
include Makefile

# Environment
MKDIR=mkdir -p
RM=rm -f 
CP=cp 
# Macros
CND_CONF=PIC18F26J50

ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof
else
IMAGE_TYPE=production
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof
endif
# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}
# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/usbdrv.o ${OBJECTDIR}/_ext/1472/usbctrltrf.o ${OBJECTDIR}/_ext/1472/usbmmap.o ${OBJECTDIR}/_ext/1472/usb9.o ${OBJECTDIR}/_ext/1472/usbdsc.o ${OBJECTDIR}/_ext/1472/hid.o ${OBJECTDIR}/_ext/1472/Boot46J50Family.o


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

OS_ORIGINAL="MINGW32_NT-6.1"
OS_CURRENT="$(shell uname -s)"
############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
MP_CC=C:\\MCC18\\bin\\mcc18.exe
MP_AS=C:\\MCC18\\bin\\..\\mpasm\\MPASMWIN.exe
MP_LD=C:\\MCC18\\bin\\mplink.exe
MP_AR=C:\\MCC18\\bin\\mplib.exe
MP_CC_DIR=C:\\MCC18\\bin
MP_AS_DIR=C:\\MCC18\\bin\\..\\mpasm
MP_LD_DIR=C:\\MCC18\\bin
MP_AR_DIR=C:\\MCC18\\bin
# This makefile will use a C preprocessor to generate dependency files
MP_CPP=C:/Program\ Files\ \(x86\)/Microchip/MPLAB\ X\ IDE/mplab_ide/mplab_ide/modules/../../bin/mplab-cpp
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-PIC18F26J50.mk dist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof

# ------------------------------------------------------------------------------------
# Rules for buildStep: createRevGrep
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
__revgrep__:   nbproject/Makefile-${CND_CONF}.mk
	@echo 'grep -q $$@' > __revgrep__
	@echo 'if [ "$$?" -ne "0" ]; then' >> __revgrep__
	@echo '  exit 0' >> __revgrep__
	@echo 'else' >> __revgrep__
	@echo '  exit 1' >> __revgrep__
	@echo 'fi' >> __revgrep__
	@chmod +x __revgrep__
else
__revgrep__:   nbproject/Makefile-${CND_CONF}.mk
	@echo 'grep -q $$@' > __revgrep__
	@echo 'if [ "$$?" -ne "0" ]; then' >> __revgrep__
	@echo '  exit 0' >> __revgrep__
	@echo 'else' >> __revgrep__
	@echo '  exit 1' >> __revgrep__
	@echo 'fi' >> __revgrep__
	@chmod +x __revgrep__
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/main.o ../main.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/main.o.temp ../main.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/main.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/main.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/main.o.d
else
	cat ${OBJECTDIR}/_ext/1472/main.o.temp >> ${OBJECTDIR}/_ext/1472/main.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbdrv.o: ../usbdrv.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbdrv.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbdrv.o ../usbdrv.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbdrv.o.temp ../usbdrv.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbdrv.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbdrv.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbdrv.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbdrv.o.temp >> ${OBJECTDIR}/_ext/1472/usbdrv.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbctrltrf.o: ../usbctrltrf.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbctrltrf.o ../usbctrltrf.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbctrltrf.o.temp ../usbctrltrf.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbctrltrf.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbctrltrf.o.temp >> ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbmmap.o: ../usbmmap.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbmmap.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbmmap.o ../usbmmap.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbmmap.o.temp ../usbmmap.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbmmap.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbmmap.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbmmap.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbmmap.o.temp >> ${OBJECTDIR}/_ext/1472/usbmmap.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usb9.o: ../usb9.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usb9.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usb9.o ../usb9.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usb9.o.temp ../usb9.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usb9.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usb9.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usb9.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usb9.o.temp >> ${OBJECTDIR}/_ext/1472/usb9.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbdsc.o: ../usbdsc.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbdsc.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbdsc.o ../usbdsc.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbdsc.o.temp ../usbdsc.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbdsc.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbdsc.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbdsc.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbdsc.o.temp >> ${OBJECTDIR}/_ext/1472/usbdsc.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/hid.o: ../hid.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/hid.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/hid.o ../hid.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/hid.o.temp ../hid.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/hid.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/hid.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/hid.o.d
else
	cat ${OBJECTDIR}/_ext/1472/hid.o.temp >> ${OBJECTDIR}/_ext/1472/hid.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/Boot46J50Family.o: ../Boot46J50Family.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/Boot46J50Family.o ../Boot46J50Family.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.temp ../Boot46J50Family.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d
else
	cat ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.temp >> ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d
endif
	${RM} __temp_cpp_output__
else
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/main.o ../main.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/main.o.temp ../main.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/main.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/main.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/main.o.d
else
	cat ${OBJECTDIR}/_ext/1472/main.o.temp >> ${OBJECTDIR}/_ext/1472/main.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbdrv.o: ../usbdrv.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbdrv.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbdrv.o ../usbdrv.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbdrv.o.temp ../usbdrv.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbdrv.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbdrv.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbdrv.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbdrv.o.temp >> ${OBJECTDIR}/_ext/1472/usbdrv.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbctrltrf.o: ../usbctrltrf.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbctrltrf.o ../usbctrltrf.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbctrltrf.o.temp ../usbctrltrf.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbctrltrf.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbctrltrf.o.temp >> ${OBJECTDIR}/_ext/1472/usbctrltrf.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbmmap.o: ../usbmmap.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbmmap.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbmmap.o ../usbmmap.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbmmap.o.temp ../usbmmap.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbmmap.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbmmap.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbmmap.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbmmap.o.temp >> ${OBJECTDIR}/_ext/1472/usbmmap.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usb9.o: ../usb9.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usb9.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usb9.o ../usb9.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usb9.o.temp ../usb9.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usb9.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usb9.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usb9.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usb9.o.temp >> ${OBJECTDIR}/_ext/1472/usb9.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/usbdsc.o: ../usbdsc.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/usbdsc.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/usbdsc.o ../usbdsc.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/usbdsc.o.temp ../usbdsc.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/usbdsc.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/usbdsc.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/usbdsc.o.d
else
	cat ${OBJECTDIR}/_ext/1472/usbdsc.o.temp >> ${OBJECTDIR}/_ext/1472/usbdsc.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/hid.o: ../hid.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/hid.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/hid.o ../hid.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/hid.o.temp ../hid.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/hid.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/hid.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/hid.o.d
else
	cat ${OBJECTDIR}/_ext/1472/hid.o.temp >> ${OBJECTDIR}/_ext/1472/hid.o.d
endif
	${RM} __temp_cpp_output__
${OBJECTDIR}/_ext/1472/Boot46J50Family.o: ../Boot46J50Family.c  nbproject/Makefile-${CND_CONF}.mk
	${RM} ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d 
	${MKDIR} ${OBJECTDIR}/_ext/1472 
	${MP_CC}  -p18F26J50  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/Boot46J50Family.o ../Boot46J50Family.c 
	${MP_CPP}  -MMD ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.temp ../Boot46J50Family.c __temp_cpp_output__ -D __18F46J50 -D __18CXX -I C:\\MCC18\\bin/../h  -D__18F46J50
	printf "%s/" ${OBJECTDIR}/_ext/1472 > ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d
ifneq (,$(findstring MINGW32,$(OS_CURRENT)))
	cat ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.temp | sed -e 's/\\\ /__SPACES__/g' -e's/\\$$/__EOL__/g' -e 's/\\/\\\\/g' -e 's/__SPACES__/\\\ /g' -e 's/__EOL__/\\/g' >> ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d
else
	cat ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.temp >> ${OBJECTDIR}/_ext/1472/Boot46J50Family.o.d
endif
	${RM} __temp_cpp_output__
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_LD} ../BootModified.rm18f46j50_g.lkr   -w -x  -z__MPLAB_BUILD=1  -u_CRUNTIME -z__MPLAB_DEBUG=1 -z__MPLAB_DEBUGGER_REAL_ICE=1  -l ${MP_CC_DIR}\\..\\lib  -odist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof ${OBJECTFILES}     
else
dist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_LD} ../BootModified.rm18f46j50_g.lkr   -w   -z__MPLAB_BUILD=1  -u_CRUNTIME -l ${MP_CC_DIR}\\..\\lib  -odist/${CND_CONF}/${IMAGE_TYPE}/MPLAB.X.${IMAGE_TYPE}.cof ${OBJECTFILES}     
endif


# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/PIC18F26J50
	${RM} -r dist
# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
