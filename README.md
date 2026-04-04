# geany-cliptext

Adds clickable snippets to the sidebar for text editing

# Compile

**Debian 13 (Trixie):**

```

sudo apt update && sudo apt install build-essential geany geany-common libgtk-3-dev libglib2.0-dev pkg-config autoconf automake autopoint;

cd /src; gcc -shared -fPIC `pkg-config --cflags --libs geany` cliptext.c -o cliptext.so;

```


Place _cliptext.so_ to _/home/user/.config/geany/plugins/_

and _cliptext.conf_ to _/home/user/.config/geany/plugins/cliptext/_

