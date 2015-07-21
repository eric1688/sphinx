################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/indexer.cpp \
../src/indextool.cpp \
../src/mkdict.cpp \
../src/searchd.cpp \
../src/spelldump.cpp \
../src/sphinx.cpp \
../src/sphinxaot.cpp \
../src/sphinxexcerpt.cpp \
../src/sphinxexpr.cpp \
../src/sphinxfilter.cpp \
../src/sphinxjson.cpp \
../src/sphinxmetaphone.cpp \
../src/sphinxplugin.cpp \
../src/sphinxquery.cpp \
../src/sphinxrt.cpp \
../src/sphinxsearch.cpp \
../src/sphinxsort.cpp \
../src/sphinxsoundex.cpp \
../src/sphinxstd.cpp \
../src/sphinxstemar.cpp \
../src/sphinxstemcz.cpp \
../src/sphinxstemen.cpp \
../src/sphinxstemru.cpp \
../src/sphinxutils.cpp \
../src/testrt.cpp \
../src/tests.cpp \
../src/wordbreaker.cpp 

C_SRCS += \
../src/llsphinxjson.c \
../src/llsphinxql.c \
../src/sphinxudf.c \
../src/udfexample.c \
../src/yysphinxexpr.c \
../src/yysphinxjson.c \
../src/yysphinxql.c \
../src/yysphinxquery.c \
../src/yysphinxselect.c 

OBJS += \
./src/indexer.o \
./src/indextool.o \
./src/llsphinxjson.o \
./src/llsphinxql.o \
./src/mkdict.o \
./src/searchd.o \
./src/spelldump.o \
./src/sphinx.o \
./src/sphinxaot.o \
./src/sphinxexcerpt.o \
./src/sphinxexpr.o \
./src/sphinxfilter.o \
./src/sphinxjson.o \
./src/sphinxmetaphone.o \
./src/sphinxplugin.o \
./src/sphinxquery.o \
./src/sphinxrt.o \
./src/sphinxsearch.o \
./src/sphinxsort.o \
./src/sphinxsoundex.o \
./src/sphinxstd.o \
./src/sphinxstemar.o \
./src/sphinxstemcz.o \
./src/sphinxstemen.o \
./src/sphinxstemru.o \
./src/sphinxudf.o \
./src/sphinxutils.o \
./src/testrt.o \
./src/tests.o \
./src/udfexample.o \
./src/wordbreaker.o \
./src/yysphinxexpr.o \
./src/yysphinxjson.o \
./src/yysphinxql.o \
./src/yysphinxquery.o \
./src/yysphinxselect.o 

C_DEPS += \
./src/llsphinxjson.d \
./src/llsphinxql.d \
./src/sphinxudf.d \
./src/udfexample.d \
./src/yysphinxexpr.d \
./src/yysphinxjson.d \
./src/yysphinxql.d \
./src/yysphinxquery.d \
./src/yysphinxselect.d 

CPP_DEPS += \
./src/indexer.d \
./src/indextool.d \
./src/mkdict.d \
./src/searchd.d \
./src/spelldump.d \
./src/sphinx.d \
./src/sphinxaot.d \
./src/sphinxexcerpt.d \
./src/sphinxexpr.d \
./src/sphinxfilter.d \
./src/sphinxjson.d \
./src/sphinxmetaphone.d \
./src/sphinxplugin.d \
./src/sphinxquery.d \
./src/sphinxrt.d \
./src/sphinxsearch.d \
./src/sphinxsort.d \
./src/sphinxsoundex.d \
./src/sphinxstd.d \
./src/sphinxstemar.d \
./src/sphinxstemcz.d \
./src/sphinxstemen.d \
./src/sphinxstemru.d \
./src/sphinxutils.d \
./src/testrt.d \
./src/tests.d \
./src/wordbreaker.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


