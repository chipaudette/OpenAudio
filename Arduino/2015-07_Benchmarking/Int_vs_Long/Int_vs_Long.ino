

//include <mult16x16.h>
//include <mult16x8.h>
//include <mult32x16.h>

// signed16 * signed 16 >> 16
#define MultiS16X16toH16(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"muls %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mulsu %B2, %A1 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26", "r27" \
)

int int_val = 0;
int out_val;
long long_val = 0;

void setup() {
  Serial.begin(115200);
  
  // put your setup code here, to run once:
  int_val = -32000;
  Serial.print("Int = ");
  Serial.println(int_val);
  Serial.print("Int+Int = ");
  Serial.println(int_val+int_val);
  Serial.print("long(Int)+long(Int) = ");
  Serial.println(long(int_val)+long(int_val));
  Serial.print("int(long(Int)+long(Int)) = ");
  Serial.println(int(long(int_val)+long(int_val)));
  Serial.print("int( (long(Int)+long(Int)) >> 16 ) = ");
  Serial.println(int( (long(int_val)+long(int_val)) >> 16 ));

  Serial.println("*********************");
  Serial.print("int_val = ");
  Serial.println(int_val);
  Serial.print("int_val * -int_val = ");
  Serial.println(int_val * -int_val);
  Serial.print("long(int_val) * long(-int_val) = ");
  Serial.println(long(int_val) * long(-int_val));
  Serial.print("int( long(int_val) * long(-int_val) >> 16) = ");
  Serial.println(int( long(int_val) * long(-int_val) >> 16));
  Serial.print("int( MultiS16X16toH16(int_val, intVal) = ");
  MultiS16X16toH16(out_val,int_val,-int_val);
  Serial.println(out_val);
    

}

long start_micros, end_micros;
#define N_LOOP 320000L
void loop() {
  // put your main code here, to run repeatedly:

  start_micros = micros();
  
  for (long i=0; i<N_LOOP; i++) {
    out_val = long(int(i)) * long(-int_val) >> 16;
  }

  end_micros = micros();
  Serial.print("Long,Long,Shift = ");
  Serial.print(float(end_micros-start_micros)/float(N_LOOP));
  Serial.println(" micros");

  ////////////////////////////

  start_micros = micros();
  
  for (long i=0; i<N_LOOP; i++) {
    MultiS16X16toH16(out_val,int(i),-int_val);
  }

  end_micros = micros();
  Serial.print("Asm = ");
  Serial.print(float(end_micros-start_micros)/float(N_LOOP));
  Serial.println(" micros");
  
}
