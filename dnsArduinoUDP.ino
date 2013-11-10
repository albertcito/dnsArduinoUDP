#include <SPI.h> // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#define UDP_TX_PACKET_MAX_SIZE 250

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x82, 0xC9 };
IPAddress ip(10,10,7,10);

unsigned int localPort = 53; // local port to listen on

byte packetBuffer[UDP_TX_PACKET_MAX_SIZE];
byte ReplyBuffer[UDP_TX_PACKET_MAX_SIZE];

String dominio = "";
int n_puntos = 0; // por ejemplo cliente.grupo.redes tiene 2 puntos

IPAddress ip_servidor_dns(146,83,183,94);

IPAddress ip_mi_maquina(10,10,7,1);
int puerto_en_espera = 0;

EthernetUDP Udp;

int mi_ip_servidor[4] = {10,10,7,1};


//Dominios que estan en mi servidor
int    type_A_n = 4;
String type_A[4] = {"cliente1","cliente2","cliente3","cliente4"};

//subdominios que estan en mi servidor
int    type_CN_n = 3;
String type_CNAME[3] = {"sub1","sub2","sub3"};

//subdominios que estan en mi servidor
String type_MX = "mail";

//name server
String NS = "ns";

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
        
    dominio = getDominio(packetBuffer);
    n_puntos = cuentaOcurrencias( dominio, '.' );
    int es_query = esQuery();

    int mi_grupo = esMiGrupo(7);
    /**
     Serial.print("  ");
     Serial.print(" >> pts: ");
     Serial.print(n_puntos);     
     Serial.print( " =>  mi_grupo: " );
     Serial.print( mi_grupo ); 
     Serial.print( " =>  dom: " );
     Serial.print(dominio);
     Serial.print( " =>  es_query: " );
     Serial.print( es_query );
     Serial.println( " / " );
     /**/
     
    if( es_query == 0 ){ // nunca me envian una respuesta así se obvia que mis envíos al router padre no son contestados o el arduino no los puede recibir
       Serial.println("Query 0 :respuesta");
      
        Udp.beginPacket(ip_mi_maquina, puerto_en_espera);
        for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) Udp.write((uint8_t)packetBuffer[i]);
        Udp.endPacket();
      
    } else{ //entonces es una pregunta
      if( mi_grupo == 1 ){ //si es mi grupo consulta por ello
     
        procesaPaqueteDNS();
        
      } else{
          sendAlRouter();
      }
    }
/**
int d = esUnGrupo();
String resu = (d == 1)? "Si pertenece a un Grupo en la red":"No pertenece a un Grupo en la red";
Serial.println(resu);
/**/
   
  }
  delay(10);
}

/**
 * Retorna si la consulta es pregunta 1 o respuesta 0
 **/
int esQuery(){
  int b = packetBuffer[5]; //QDCOUNT
  return b;
} 

/**
 * Si no es mi grupo envia la consulta al Router
 **/
void sendAlRouter(){
  
  if( esUnGrupo() == 1 ){ //verifica si es del tipo ....AAA.BBBB.grupoXX.redes
        
   puerto_en_espera = Udp.remotePort();
   
    Udp.beginPacket(ip_servidor_dns, 53);
    for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) Udp.write((uint8_t)packetBuffer[i]);
    Udp.endPacket();
    
  }
  
}

/***
*
*  Esta función crea y envía la respuesta.
*
* Para un dominio de este tipo: !clienteX.grupoXX.redes0 (24 byte) el Paquete DNS debe tener las siguientes posiciones
*
* ID = ReplyBuffer[0] y ReplyBuffer[1]
* QR = ReplyBuffer[2]*2^7
* QPCODE = ReplyBuffer[2]*pow(2,6) + ReplyBuffer[2]*pow(2,5) + ReplyBuffer[2]*pow(2,4) + ReplyBuffer[2]*pow(2,3)
* AA = ReplyBuffer[2]*2^2
* TC = ReplyBuffer[2]*2^1
* RA = ReplyBuffer[2]*2^0
* res1 = ReplyBuffer[3]
* res2 = ReplyBuffer[3]
* res3 = ReplyBuffer[3]
* RCODE = ReplyBuffer[3]
* QDCOUNT = ReplyBuffer[4] y ReplyBuffer[5]
* ANCOUNT = ReplyBuffer[6] y ReplyBuffer[7]
* NSCOUNT = ReplyBuffer[8] y ReplyBuffer[9]
* ARCOUNT = ReplyBuffer[10] y ReplyBuffer[11]
// Question
* QNAME = desde ReplyBuffer[12] al ReplyBuffer[35] // [29] si es NS
* QTYPE = ReplyBuffer[36] y ReplyBuffer[37]  // [30] [31] si es NS
* QCLASS = ReplyBuffer[38] y ReplyBuffer[39] // [32] [33] si es NS
//Answer
* NAME = ReplyBuffer[40] y ReplyBuffer[41] (192 y 12) puntero // [34] [35] si es NS
* TYPE = ReplyBuffer[42] y ReplyBuffer[43]
* CLASS = ReplyBuffer[44] y ReplyBuffer[45]
* TTL = ReplyBuffer[46] y ReplyBuffer[49]
* RLENGTH = ReplyBuffer[50] y ReplyBuffer[51]
* RDATA = ReplyBuffer[52] al ReplyBuffer[55]
*
**/

void procesaPaqueteDNS(){
  
  int es_ns = -1;
  
  if ( esTypeA() != 1 ){ //si no es un type A entonces esta mal escrito el dominio
    es_ns = esNS();
    if( !(es_ns == 1) )  return; //no consultan por un A ni por un NS
  } 
  makeRespuestaBase(); //creo la base para la respuesta
   
  int cname = tieneCName();   
  // Quiere decir que tiene más de 1 subdominio y mi servidor solo soporta subdominos primeros 
   if( cname == -2 ){
     cname = tieneMX(); //puede que no sea un CNAME pero si un MX
     if( cname == -3 )  return; 
   }
   
   addTypeA(cname,es_ns);
   addCNAMEoMX(cname,es_ns);
   addNS(cname,es_ns);
   
   sendDNSPackage();
 
}
/**
 * Envía el paquete DNS
 **/
void sendDNSPackage(){
  
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) Udp.write((uint8_t)ReplyBuffer[i]);
  Udp.endPacket();

}
/**
 * 
 * Crea la base de la respuesta, luego voy llenando dependiendo de la consulta
 *
 **/
void makeRespuestaBase(){
   //copio la pregunta en la respuesta
  for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) ReplyBuffer[i] = packetBuffer[i];
  //                 QR           OPCODE                           OPCODE                           OPCODE                         OPCODE                 AA           TC                    RA
  ReplyBuffer[2] = 1*128 + bitRead(packetBuffer[2],6)*64 + bitRead(packetBuffer[2],5)*32 + bitRead(packetBuffer[2],4)*16+ bitRead(packetBuffer[2],3)*8 + 1*4 + bitRead(packetBuffer[2],1)*2 + 1;
  
  ReplyBuffer[6] = 0; //ANCOUNT
  ReplyBuffer[7] = 1; //ANCOUNT  
}
/**
 * 
 * Agrega a la respuesta el Type A
 *
 **/
void addTypeA(int cname, int mi_ns){

  int j = 0;
  if( cname == 1 || cname == 2 ) j = 5;
  if( mi_ns == 1) {
    addNSTypeA();
    return;
  }

   //Answer
   ReplyBuffer[40+j] = 192; //NAME 11000000
   ReplyBuffer[41+j] = 12+j; //NAME dirección donde esta el QNAME
   
   ReplyBuffer[42+j] = 0; //TYPE
   ReplyBuffer[43+j] = 1; //TYPE es tipo A
   ReplyBuffer[44+j] = packetBuffer[38+j]; //CLASS
   ReplyBuffer[45+j] = packetBuffer[39+j]; //CLASS

   ReplyBuffer[46+j] = 0; //TTL
   ReplyBuffer[47+j] = 0; //TTL
   ReplyBuffer[48+j] = 0; //TTL
   ReplyBuffer[49+j] = 60; //TTL

   ReplyBuffer[50+j] = 0; //RLENGTH
   ReplyBuffer[51+j] = 4; //RLENGTH
   
   ReplyBuffer[52+j] = mi_ip_servidor[0];
   ReplyBuffer[53+j] = mi_ip_servidor[1];
   ReplyBuffer[54+j] = mi_ip_servidor[2]; //id_grupo; //número de Grupo
   ReplyBuffer[55+j] = mi_ip_servidor[3]; //id_cliente; //número de Cliente
  /**/
}

/**
* Se envía un NS
**/
void addNSTypeA(){
 
   ReplyBuffer[34] = 192; //NAME 11000000
   ReplyBuffer[35] = 12; //NAME dirección donde esta el QNAME
   
   ReplyBuffer[36] = 0; //TYPE
   ReplyBuffer[37] = 2; //TYPE es tipo NS
   ReplyBuffer[38] = 0; //CLASS
   ReplyBuffer[39] = 1; //CLASS

   ReplyBuffer[40] = 0; //TTL
   ReplyBuffer[41] = 0; //TTL
   ReplyBuffer[42] = 0; //TTL
   ReplyBuffer[43] = 60; //TTL

   ReplyBuffer[44] = 0; //RLENGTH
   ReplyBuffer[45] = 2; //RLENGTH
   
   ReplyBuffer[46] = 192;
   ReplyBuffer[47] = 12;
  
  
}
/**
 * 
 * Agrega a la respuesta el CNAME
 *
 **/
void addCNAMEoMX(int cname, int mi_ns){

  if( mi_ns == 1 ) return;
  
  if( !(cname == 1 || cname == 2) ) return;

   ReplyBuffer[7]  = 2; // ahora son 2 respuestas 
   
   ReplyBuffer[61] = 192;
   ReplyBuffer[62] = 12;
   
   ReplyBuffer[63] = 0; //TYPE
   if( cname == 1 )  ReplyBuffer[64] = 5; //TYPE es tipo CNAME
   else  if( cname == 2 )  ReplyBuffer[64] = 15; //TYPE es tipo MX
   
   ReplyBuffer[65] = packetBuffer[38+5]; //CLASS
   ReplyBuffer[66] = packetBuffer[39+5]; //CLASS
   
   ReplyBuffer[67] = 0; //TTL
   ReplyBuffer[68] = 0; //TTL
   ReplyBuffer[69] = 0; //TTL
   ReplyBuffer[70] = 60; //TTL
   
   ReplyBuffer[71] = 0; //RLENGTH
   
   if( cname == 1 ){  
     
     ReplyBuffer[72] = 2; //RDLENGTH es tipo CNAME
     ReplyBuffer[73] = 192;
     ReplyBuffer[74] = 17; //DATA es tipo CNAME
     
   }
   else  if( cname == 2 ){
     ReplyBuffer[72] = 4; //RDLENGTH es tipo MX
     ReplyBuffer[73] = 0; //Preference
     ReplyBuffer[74] = 4; //Preference
     ReplyBuffer[75] = 192; //Puntero a Mail Exchanger
     ReplyBuffer[76] = 12; //Dominio actual
   }    

   
  
}
/**
 * 
 * Agrega a la respuesta el NS
 *
 **/
void addNS(int cname, int mi_ns){
  
  if( mi_ns == 1 ) return;
  
  ReplyBuffer[9]  = 1; // ahora hay una respuesta autoritative
  
  int j = 0;
  int dom = 0;
  if( cname == 1 || cname == 2){
    j = 19;
    dom = 5;
    if( cname == 2 ) j = 21;
  }
  if( mi_ns == 1) j = -10;
  
   ReplyBuffer[56+j] = 192;
   ReplyBuffer[57+j] = 12 + dom;
   
   ReplyBuffer[58+j] = 0; //TYPE 
   ReplyBuffer[59+j] = 2; //TYPE es tipo NS
   
   ReplyBuffer[60+j] = 0; //CLASS
   ReplyBuffer[61+j] = 1; //CLASS
   
   ReplyBuffer[62+j] = 0; //TTL
   ReplyBuffer[63+j] = 0; //TTL
   ReplyBuffer[64+j] = 0; //TTL
   ReplyBuffer[65+j] = 60; //TTL
   
   ReplyBuffer[66+j] = 0; //RLENGTH
   ReplyBuffer[67+j] = 5; //RLENGTH

   ReplyBuffer[68+j] = 2;
   ReplyBuffer[69+j] = 'n';
   ReplyBuffer[70+j] = 's';
   ReplyBuffer[71+j] = 192;
   ReplyBuffer[72+j] = 21 + dom;  
  
  
}









/**
 * Retorna si es un NS de nuestro servidor
 **/
int esNS(){
  if( n_puntos == 2 ){ //puede ser ns.grupo07.redes que es lo máximo permitido
    String my_NS = getValor(dominio, '.', n_puntos - 2); //obtengo el NS = email (ns.grupo07.redes)     
    if( NS == my_NS ) return 1;
    return -2; // no es NS el ingresado
  }
  return -1;
}



/**
 * Retorna si es un MX en nuestro servidor
 **/
int tieneMX(){
 
  if( n_puntos == 3 ){ //puede ser email.cliente.grupo07.redes que es lo máximo permitido
    String MX = getValor(dominio, '.', n_puntos - 3); //obtengo el CN = email (email.cliente1.grupo07.redes) 
    //for( int j = 0; j < type_MX_n; j++ ){      ;
      //if( type_MX[j] == MX ) return 2;
    //}
    if( type_MX == MX ) return 2;
    return -3; // hay un subdominio pero no existe en la tabla de subdominios
  }
  return -1;
}
/**
 * Retorna si es un subdominio dominio en nuestro servidor
 **/
int tieneCName(){
 
  if( n_puntos == 3 ){ //puede ser subdom.cliente.grupo07.redes que es lo máximo permitido
    String CN = getValor(dominio, '.', n_puntos - 3); //obtengo el CN = subdom (subdom.dominio.grupo07.redes)
    for( int j = 0; j < type_CN_n; j++ ){
      if( type_CNAME[j] == CN ) return 1;
    }
    return -2; // hay un subdominio pero no existe en la tabla de subdominios
  }
  return -1;
}


/**
 * Retorna si es un dominio en nuestro servidor
 **/
int esTypeA(){

  if( n_puntos >= 2 ){ //puede ser cliente.grupo07.redes
    String A = getValor(dominio, '.', n_puntos - 2); //obtengo el A = dominio (dominio.grupo07.redes)   
    for( int i = 0; i < type_A_n; i++ ){
      if( type_A[i] == A ) return 1;
    }
  }
  return -1;
}







/**
* Obtiene el dominio del arreglo
**/
String getDominio(byte packetBuffer[]){
  String dominnio = "";
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


int esMiGrupo(int mi_grupo){
  return esGrupo(mi_grupo);
}
int esUnGrupo(){
  int sin_grupo = -1;
  return esGrupo(sin_grupo);
}
 
/**
* Entrega un dominio y devuelve si es una maquina perteneciente a mi grupo clienteX.grupo07.redes
* Si ingresa en grupo un valor negativo verificará si es que pertenece a cualquier grupo
**/
int esGrupo(int grupo){

  if( n_puntos < 2 ) return -1;
  
  //verifico REDES
  String p2 = getValor(dominio, '.', n_puntos);  
  if( p2 != "redes") return -2;  
  
  //Verifico grupoXX
  String p1 = getValor(dominio, '.', n_puntos-1);
  if( p1.length() != 7 ) return -1;
  if( p1.substring(0,5) != "grupo" ) return -4;
  int num_grupo = numGrupo(p1);
  if( num_grupo == -1 ) return -1;
  // Aquí consulto si es de mi grupo
  if( grupo > 0 && num_grupo != grupo ) return -5;

  return 1;
}


/**
* Obtiene el numero del grupo
**/
int getGrupo(){
  String p1 = getValor(dominio, '.', 1);
  int num_grupo = numGrupo(p1);
  return num_grupo;
}

/**
* devuelve el número del grupo o cliente solo puede ser entre 0 y 99
**/
int numGrupo(String num){
  
  if( num.length() != 7) return -1;
  
  int p_num = (int)num.charAt(5) - 48;
  int s_num = (int)num.charAt(6) - 48;
  
  //solo pueden ser números de 0 al 10
  if( p_num < 10 && p_num > -1 &&
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
