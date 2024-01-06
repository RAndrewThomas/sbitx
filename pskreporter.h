struct pskr_spot
{
    char  callsign[17];
    char  gridloc [ 7];
    char  time_str[20];

    float freq_hz;
    int   snr;

    struct pskr_spot *next_spot;
};




int pskr_add_spot(char *time, int16_t snr, float hz, char *msg);

int pskr_get_num_spots(void);
char *pskr_get_spot(void);
int pskr_open(void);
int pskr_close(void);





