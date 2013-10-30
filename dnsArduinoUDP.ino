#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008     
#define UDP_TX_PACKET_MAX_SIZE 512

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0D, 0x82, 0xC9 };
IPAddress ip(10,10,7,10);

unsigned int localPort = 53;      // local port to listen on


byte packetBuffer[UDP_TX_PACKET_MAX_SIZE]; 
byte ReplyBuffer[UDP_TX_PACKET_MAX_SIZE]; 
String  dominio = "";

int salida[32];

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);

  Serial.begin(9600);
}

void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize){ 
   
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE); 
    
    Serial.println("QNAME:");  
    dominio = getDominio(packetBuffer);
    Serial.println( dominio );
    
    int mi_grupo = esMiGrupo();
    if( mi_grupo == 1 ){
      
      crearPaqueteDNS();
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) Udp.write((uint8_t)ReplyBuffer[i]);
      Udp.endPacket();

    } else{
      
      IPAddress ip_servidor_dns(146.83.216.250);      
      Udp.beginPacket(ip_servidor_dns, 53);
      for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) Udp.write((uint8_t)packetBuffer[i]);
      Udp.endPacket();
      Udp.read(ReplyBuffer,UDP_TX_PACKET_MAX_SIZE); 
      
    }
    /**
    int d = esUnGrupo();
    String resu = (d == 1)? "Si pertenece a un Grupo en la red":"No pertenece a un Grupo en la red";
    Serial.println(resu);
    /**/    
   
  }
  delay(10);
}



/***
 * Para un dominio de este tipo: !clienteX.grupoXX.redes0 (24 byte) el Paquete DNS debe tener las siguientes posiciones
 *
 *  ID = ReplyBuffer[0] y ReplyBuffer[1] 
 
 *  QR = ReplyBuffer[2]*2^7 
 *  QPCODE =  ReplyBuffer[2]*pow(2,6) +  ReplyBuffer[2]*pow(2,5) +  ReplyBuffer[2]*pow(2,4) +  ReplyBuffer[2]*pow(2,3)
 *  AA = ReplyBuffer[2]*2^2
 *  TC = ReplyBuffer[2]*2^1
 *  RA = ReplyBuffer[2]*2^0
 
 *  res1  = ReplyBuffer[3]
 *  res2  = ReplyBuffer[3]
 *  res3  = ReplyBuffer[3]
 *  RCODE = ReplyBuffer[3]
 
 *  QDCOUNT = ReplyBuffer[4] y ReplyBuffer[5]
 *  ANCOUNT = ReplyBuffer[6] y ReplyBuffer[7]
 *  NSCOUNT = ReplyBuffer[8] y ReplyBuffer[9]
 *  ARCOUNT = ReplyBuffer[10] y ReplyBuffer[11]
 
 // Question
 
 * QNAME = desde ReplyBuffer[12] al ReplyBuffer[35]
 * QTYPE = ReplyBuffer[36] y ReplyBuffer[37]
 * QCLASS = ReplyBuffer[38] y ReplyBuffer[39]
 
 //Answer
 
 * NAME = ReplyBuffer[40] y ReplyBuffer[41] (192 y 12) puntero
 * TYPE = ReplyBuffer[42] y ReplyBuffer[43]
 * CLASS = ReplyBuffer[44] y ReplyBuffer[45]
 * TTL = ReplyBuffer[46] y ReplyBuffer[49]
 * RLENGTH  = ReplyBuffer[50] y ReplyBuffer[51]
 * RDATA  = ReplyBuffer[52] al ReplyBuffer[55]
 *
 **/

void crearPaqueteDNS(){
  
   //copio la pregunta en la respuesta
   for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) ReplyBuffer[i] = packetBuffer[i];
   
   int id_cliente = getCliente();
   int id_grupo = getGrupo();
   
   // -- Empiezo a modificar la respuesta para que quede como respuesta según el RFC 1035 --
  
   //                  QR              OPCODE                            OPCODE                        OPCODE                      OPCODE                    AA             TC                   RA
   ReplyBuffer[2] = 1*128 +  bitRead(packetBuffer[2],6)*64  +  bitRead(packetBuffer[2],5)*32 +  bitRead(packetBuffer[2],4)*16+  bitRead(packetBuffer[2],3)*8 +  1*4 +  bitRead(packetBuffer[2],1)*2 +  1;
  
   ReplyBuffer[6] = 0; //ANCOUNT
   ReplyBuffer[7] = 1; //ANCOUNT
  
   //Answer
   ReplyBuffer[40] = 192; //NAME 11000000
   ReplyBuffer[41] = 12;  //NAME dirección donde esta el QNAME
   
   ReplyBuffer[42] = packetBuffer[36]; //TYPE 
   ReplyBuffer[43] = packetBuffer[37]; //TYPE
   ReplyBuffer[44] = packetBuffer[38]; //CLASS
   ReplyBuffer[45] = packetBuffer[39]; //CLASS
   
   ReplyBuffer[46] = 0; //TTL
   ReplyBuffer[47] = 0; //TTL
   ReplyBuffer[48] = 0; //TTL
   ReplyBuffer[49] = 60; //TTL

   ReplyBuffer[50] = 0; //RLENGTH
   ReplyBuffer[51] = 4; //RLENGTH
   
   ReplyBuffer[52] = 10;
   ReplyBuffer[53] = 10;
   ReplyBuffer[54] = id_grupo; //id_grupo; //número de Grupo
   ReplyBuffer[55] = id_cliente; //id_cliente; //número de Cliente

}









































/**
 * Obtiene el dominio del arreglo 
 **/
String getDominio(byte packetBuffer[]){
  String  dominnio = "";
  int i = 13;
  while(true){     
    int aux = (char)packetBuffer[i];
     if( aux < 47 ){
       dominnio = dominnio + ".";      
       i++;
     }else{
       dominnio = dominnio + String((char)packetBuffer[i++]);
     }     
     if( packetBuffer[i] == 0 ){
       break;
     }
  }  
   return dominnio; 
}


int esMiGrupo(){
  int mi_grupo = 7;
  return esGrupo(mi_grupo);
}
int esUnGrupo(){
  int sin_grupo = -1;
  return esGrupo(sin_grupo);
}
 
/**
 * Entrega un dominio  y devuelve si es una maquina perteneciente a mi grupo clienteX.grupo07.redes
 * Si ingresa en grupo un valor negativo verificará si es que pertenece a cualquier grupo
 **/
int esGrupo(int grupo){

  int n = cuentaOcurrencias( dominio, '.' );
   
  if( n != 2 ) return -1;
  
  //verifico REDES
  String p2 = getValor(dominio, '.', 2);
  if( p2 != "redes") return -1;
  
  //Verifico ClienteX
  String p0 = getValor(dominio, '.', 0); 
  String cliente = p0.substring(0,7);
  if( cliente != "cliente" ) return -1;
  
  //Verifico grupoXX
  String p1 = getValor(dominio, '.', 1);
  if( p1.length() != 7 ) return -1;
  if( p1.substring(0,5) != "grupo" ) return -1;
  int num_grupo =  numGrupo(p1);
  if( num_grupo == -1 ) return -1;
  // Aquí consulto si es de mi grupo
  if( grupo > 0 && num_grupo != grupo ) return -1;
    
  return 1;  
}

/**
 * Obtiene el numero del cliente 
 **/
int getCliente(){
  String p0 = getValor(dominio, '.', 0);
  int num_grupo =  numCliente(p0);
  return num_grupo;
}

/**
 * Obtiene el numero del grupo 
 **/
int getGrupo(){
  String p1 = getValor(dominio, '.', 1);
  int num_grupo =  numGrupo(p1);
  return num_grupo;
}
/**
 * devuelve el número del grupo o cliente solo puede ser entre 0 y 99
 **/
int numCliente(String num){

  if( num.length() != 8 ) return -1;
  
  int c_num = (int)num.charAt(7) - 48;
  
  //solo pueden ser números de 0 al 10
  if( c_num < 10 && c_num > -1 )  return c_num;
  return -1;
}

/**
 * devuelve el número del grupo o cliente solo puede ser entre 0 y 99
 **/
int numGrupo(String num){
  
  if( num.length() != 7) return -1;
  
  int p_num = (int)num.charAt(5) - 48;
  int s_num = (int)num.charAt(6) - 48;
  
  //solo pueden ser números de 0 al 10
  if( p_num < 10 && p_num > -1  && 
      s_num < 10 && s_num > -1){
    
    p_num = p_num*10;    
    return p_num + s_num;
  
  }   
  return -1;
}

/**
 * Calcula el número de ocurrencias de un caracter en un String
 **/
int cuentaOcurrencias( String str, char c )
{
    int count = 0;
    for( int i=0; i < str.length(); i++ ){
      count += (str.charAt(i) == c);
    }
      
    return count;
}
/**
 * Retorna una palabra del String dependiendo de la posición y el separador
 **/
String getValor(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
