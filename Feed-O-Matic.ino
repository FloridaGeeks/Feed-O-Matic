//Libraries included
#include <Servo.h>
#include <TimerOne.h>
#include <Wire.h>

//Servo declaration
Servo myservo;
int servoPosicionReposo = 180;
int servoPosicionAbierto = 90;

//Variable para manejar el conteo de segundos del timer interrupt
int contador = 0;

//Variables para manejo de tiempos
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

//Variables para setear la medida de la racion
int tiempoCarga = 3000; //define cuantos milisegundos se mantendra abierto el servo
int cargasPorRacion = 3; //define cuantas veces se accionara el servo

//Variables para setear los niveles de reposicion del plato y del deposito
int nivelReposicionPlato = 18;
int nivelReposicionDeposito = 10;
int profundidadDeposito = 60;

//Variables para setear las 2 raciones diarias
int horaRacion1 = 11;
int minutosRacion1 = 30;
int horaRacion2 = 20;
int minutosRacion2 = 00;

//Variables para setear el rango minimo y maximo de lectura del sensor ultrasonico
int maximumRange = 60;
int minimumRange = 0;

//Declaracion de pines
const int PINTRIGGERDeposito = 2;
const int PINECHODeposito = 3;
const int PINTRIGGERPlato = 4;
const int PINECHOPlato = 5;
const int PINSERVO = 9;

//Conversiones entre formatos del RTC DS3231
#define DS3231_I2C_ADDRESS 0x68
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val / 10 * 16) + (val % 10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val / 16 * 10) + (val % 16) );
}

void setup() {
  Serial.begin(9600);

  myservo.attach(PINSERVO);

  pinMode(PINTRIGGERDeposito, OUTPUT);
  pinMode(PINECHODeposito, INPUT);
  pinMode(PINTRIGGERPlato, OUTPUT);
  pinMode(PINECHOPlato, INPUT);
  Wire.begin();

  Timer1.initialize(1000000); //1000000 = 1 segundo
  Timer1.attachInterrupt(AcumularSegundos);

  // set the initial time here:
  // DS3231 seconds, minutes, hours, day, date, month, year
  // setDS3231time(00,17,19,1,13,12,15);
  //

  Serial.println("Feed-O-Matic Iniciado");
  Serial.println("Ingrese alguno de los siguientes codigos si desea realizar seteos");
  Serial.println("hm= para setear la hora y los minutos del reloj (ej.: hm=17:35)");
  Serial.println("r1= para setear la hora y los minutos de la racion 1 (ej.: r1=10:30)");
  Serial.println("r2= para setear la hora y los minutos de la racion 2 (ej.: r2=18:00)");
  Serial.println("cr= para setear la cantidad de descargas por racion (ej.: cr=3)");
  LeerHora();
  EstadoParametros();
  NivelPlato();
  NivelDeposito();
  Serial.println("");
}

void loop() {
  if (Serial.available() > 0) {
    ParsearInputSerial(Serial.readString());
  }
  if (contador == 0) {
    LeerHora();
    MostrarHora();
    EvaluarEntregaAlimento();
  }
  delay(1000);
}

void AcumularSegundos() {
  if (contador != 59) {
    contador++;
  } else {
    contador = 0;
  }
}

void EvaluarEntregaAlimento() {
  if (EsHoraDeComida()) {
    Serial.println("Es horario de entrega de racion");
    if (PlatoEstaVacio()) {
      Serial.println("El plato esta vacio");
      Serial.println("Entregar racion");
      EntregarRacion();
    } else {
      Serial.println("El plato aun tiene comida");
      Serial.println("No se entrega la racion");
    }
    if (DepositoEstaVacio()) {
      Serial.println("Se requiere llenar el deposito");
    }
  }
}

bool EsHoraDeComida() {
  return (hour == horaRacion1 && minute == minutosRacion1) || (hour == horaRacion2 && minute == minutosRacion2);
}

bool LeerHora() {
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
}

bool MostrarHora() {
  Serial.println(FormatearHora(hour, minute));
}

String FormatearHora(int h, int m) {
  String hm = "";
  if (h < 10) {
    hm += "0";
  }
  hm += h;
  hm += ":";
  if (m < 10) {
    hm += "0";
  }
  hm += m;
  return hm;
}

int NivelPlato() {
  int d = GetDistance(PINTRIGGERPlato, PINECHOPlato);
  Serial.print("Nivel Plato: ");
  Serial.println(d); 
  return d; 
}

bool PlatoEstaVacio() {
  return (NivelPlato() >= nivelReposicionPlato);
}

int NivelDeposito() {
  int d = GetDistance(PINTRIGGERDeposito, PINECHODeposito);
  int nivel = (profundidadDeposito - d) * 100 / profundidadDeposito;
  Serial.print("Nivel Deposito: ");
  Serial.print(nivel);
  Serial.println("%");
  return nivel;  
}

bool DepositoEstaVacio() {
  return (NivelDeposito() <= nivelReposicionDeposito);
}

void EntregarRacion() {
  for (int i = 0; i < cargasPorRacion; i++) {
    Cargar();
    Serial.print("Carga Nro.: ");
    Serial.println(i + 1);
  }
  Serial.println("Racion entregada");
}

void Cargar() {
  myservo.write(servoPosicionReposo);
  myservo.write(servoPosicionAbierto);
  Serial.println("Abre Servo");
  delay(tiempoCarga);
  myservo.write(servoPosicionReposo);
  Serial.println("Cierra Servo");
}

void ParsearInputSerial(String s) {
  s.toLowerCase();
  String key = s.substring(0, s.indexOf("="));
  String value = s.substring(s.indexOf("=") + 1);

  if (key == "hm") {
    setDS3231time(00, value.substring(value.indexOf(":") + 1).toInt(), value.substring(0, value.indexOf(":")).toInt(), 1, 1, 1, 0);
  }
  if (key == "r1") {
    horaRacion1 = value.substring(0, value.indexOf(":")).toInt();
    minutosRacion1 = value.substring(value.indexOf(":") + 1).toInt();
  }
  if (key == "r2") {
    horaRacion2 = value.substring(0, value.indexOf(":")).toInt();
    minutosRacion2 = value.substring(value.indexOf(":") + 1).toInt();
  }
  if (key == "cr") {
    cargasPorRacion = value.toInt();
  }
  EstadoParametros();
}

void EstadoParametros() {
  Serial.println();
  Serial.println("Estado parametros: ");
  Serial.print("Hora Actual: " );
  MostrarHora();
  Serial.print("Hora Racion 1: ");
  Serial.println(FormatearHora(horaRacion1, minutosRacion1));
  Serial.print("Hora Racion 2: ");
  Serial.println(FormatearHora(horaRacion2, minutosRacion2));
  Serial.print("Cargas por Racion: ");
  Serial.println(cargasPorRacion);
  Serial.println();
}

//   METODOS AUXILIARES     //////////////////////////////////////////////////////

int GetDistance(int PINTRIGGER, int PINECHO) {
  long duration;
  int distance;

  digitalWrite(PINTRIGGER, LOW);
  delayMicroseconds(2);

  digitalWrite(PINTRIGGER, HIGH);
  delayMicroseconds(10);

  digitalWrite(PINTRIGGER, LOW);
  duration = pulseIn(PINECHO, HIGH);

  //Calculate the distance (in cm) based on the speed of sound.
  distance = duration / 58.2;

  if (distance <= maximumRange || distance >= minimumRange) {
    return distance;
  } else {
    return -1;
  }
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
                   dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void readDS3231time(byte *second,
                    byte *minute,
                    byte *hour,
                    byte *dayOfWeek,
                    byte *dayOfMonth,
                    byte *month,
                    byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

