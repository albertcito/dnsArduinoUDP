#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#define UDP_TX_PACKET_MAX_SIZE 70

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0D, 0x82, 0xC9 };
IPAddress ip(10,10,7,10);

unsigned int localPort = 53;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";       // a string to send back

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
  if(packetSize)
  { 
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    /**
    //pagina 25  rfc1035 ID
    Serial.println( packetBuffer[0]*256 + packetBuffer[1] );
    //pagina 25  rfc1035 QR
    Serial.println( bitRead(packetBuffer[2],7) );
    //pagina 25  rfc1035 Opcode
    Serial.println( bitRead(packetBuffer[2],7) );
    /**/
    Serial.println("QNAME:");  
    String  dominio = getDominio();
    Serial.println( dominio );
    
    int c = esGrupo( dominio, 7 );
    String res = (c == 1)? "Si pertenece a mi Grupo":"No pertenece a mi Grupo";
    Serial.println(res);
    
    int d = esGrupo( dominio, -1 );
    String resu = (d == 1)? "Si pertenece a un Grupo en la red":"No pertenece a un Grupo en la red";
    Serial.println(resu);
    
    
    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
       
  }
  delay(10);
}
/**
 * Obtiene el dominio del arreglo 
 **/
String getDominio(){
  String  dominio = "";
  int i = 13;
  while(true){     
    int aux = packetBuffer[i];
     if( aux < 47 ){
       dominio = dominio + ".";      
       i++;
     }else{
       dominio = dominio + String(packetBuffer[i++]);
     }     
     if( packetBuffer[i] == 0 ) break;
  }  
   return dominio; 
}
  
/**
 * Entrega un dominio  y devuelve si es una maquina perteneciente a mi grupo clienteX.grupo07.redes
 * Si ingresa en grupo un valor negativo verificará si es que pertenece a cualquier grupo
 **/
int esGrupo(String dominio, int grupo){

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
 * devuelve el número del grupo o cliente solo puede ser entre 0 y 99
 **/
int numGrupo(String num){
  
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
