/* This structure holds the PNG image of the QR code */
struct mem_encode
{
   char *buffer;
   size_t size;
};

int generate_qr_code(char*); 
