
# Nota: le dimensioni dei terminali valgono per la risoluzione 1920x1440

make
read -p "Compilazione eseguita. Premi invio per eseguire... "

# Esecuzione del server sulla porta 4242
gnome-terminal --geometry 190x33+0+9999 -- "./server 4242"

# Esecuzione del client (comunica sulla porta 4242)
gnome-terminal --geometry 93x33+0+0 -- "./client"

# Esecuzione del terzo device (sempre un client, sempre sulla porta 4242)
gnome-terminal --geometry 93x33+9999+0 -- "./client"
