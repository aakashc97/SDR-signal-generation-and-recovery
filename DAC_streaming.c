#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <iio.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

/* helper macros */
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))

#define BUFF_SZ (1024*64) //size of buffer
#define pi 3.141593

int input_wave[BUFF_SZ];

/* IIO structs required for streaming */
static struct iio_context *ctx   = NULL;
static struct iio_channel *rx0_i = NULL;
static struct iio_channel *rx0_q = NULL;
static struct iio_channel *tx0_i = NULL;
static struct iio_channel *tx0_q = NULL;
static struct iio_buffer  *rxbuf = NULL;
static struct iio_buffer  *txbuf = NULL;


/* cleanup and exit */
static void shutdown()
{
	printf("* Destroying buffers\n");
	if (rxbuf) { iio_buffer_destroy(rxbuf); }
	if (txbuf) { iio_buffer_destroy(txbuf); }

	printf("* Disabling streaming channels\n");
	if (rx0_i) { iio_channel_disable(rx0_i); }
	if (rx0_q) { iio_channel_disable(rx0_q); }
	if (tx0_i) { iio_channel_disable(tx0_i); }
	if (tx0_q) { iio_channel_disable(tx0_q); }

	printf("* Destroying context\n");
	if (ctx) { iio_context_destroy(ctx); }
	exit(0);
}

static bool stop;
static void handle_sig(int sig)
{
	printf("Waiting for process to finish... Got signal %d\n", sig);
	stop = true;
}

int main (int argc, char **argv)
{
	// Streaming devices
	struct iio_device *tx;
	struct iio_device *rx;
	// RX and TX sample counters
	size_t nrx = 0;
	size_t ntx = 0;
	int i,j;
	int count;
	int ampl = 32767;

	signal(SIGINT, handle_sig);

	printf("* Acquiring IIO context\n");
	ctx = iio_create_default_context();
	const char *name = iio_context_get_name(ctx);
	printf("%s\n", name);

	printf("No. of devices = %d\n", iio_context_get_devices_count(ctx));

	struct iio_device *dev1 =  iio_context_get_device(ctx, 0);
	const char *name_dev1 = iio_device_get_name(dev1);
	printf("Device 1 name : %s\n", name_dev1);

	struct iio_device *dev2 =  iio_context_get_device(ctx, 1);
	const char *name_dev2 = iio_device_get_name(dev2);
	printf("Device 2 name : %s\n", name_dev2);

	struct iio_device *dev3 =  iio_context_get_device(ctx, 2);
	const char *name_dev3 = iio_device_get_name(dev3);
	printf("Device 3 name : %s\n", name_dev3);

	//displaying channel names of RX device
	printf("No. of channels in rx = %d\n", iio_device_get_channels_count(dev2));
	for (unsigned int i=0;i<iio_device_get_channels_count(dev2);i++)
	{
		struct iio_channel *chan = iio_device_get_channel(dev2, i);
		const char *chan_nm = iio_channel_get_id(chan);
		printf("%s\n", chan_nm);
	}

	//displaying channel names of TX device
	printf("No. of channels in tx = %d\n", iio_device_get_channels_count(dev3));
	for (unsigned int i=0;i<iio_device_get_channels_count(dev3);i++)
	{
		struct iio_channel *chan = iio_device_get_channel(dev3, i);
		const char *chan_nm = iio_channel_get_id(chan);
		printf("%s\n", chan_nm);
	}
	tx = iio_context_get_device(ctx, 1);
	rx = iio_context_get_device(ctx,2);
	tx0_i = iio_device_get_channel(tx,0);
	tx0_q = iio_device_get_channel(tx,1);
	rx0_i = iio_device_get_channel(rx,0);
	rx0_q = iio_device_get_channel(rx,1);

	printf("* Enabling IIO streaming channels\n");
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);
	iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);

	printf("* Creating non-cyclic IIO buffers with 1 MiS\n");
	/*rxbuf = iio_device_create_buffer(rx, BUFF_SZ, false);
	if (!rxbuf)
	{
		perror("Could not create RX buffer");
		shutdown();
	}*/

	txbuf = iio_device_create_buffer(tx, BUFF_SZ, false);
	if (!txbuf)
	{
		perror("Could not create TX buffer");
		shutdown();
	}



	// half size is used because it was observed that there was a mismatch in speed in generating samples by hardware 
	// and reading values from software
	// due to which there was overlap in samples in parts of consecutive buffers
	int input_wave[BUFF_SZ/2]; 
	input_wave = malloc(BUFF_SZ/2*sizeof(int16_t));
	int range=100;
	int offset=0;
	int rand_num;
	
	FILE *fp;
	fp = fopen("random_ampl_30000_10BPS.bin", "rb");
	    if (fp == NULL) {
	        printf("Error opening file.");
	        return 1;
	    }
	for (int i = 0; i <1024*32; i++)
	{
	   fread(&input_wave[i], sizeof(int), 1, fp);
	}
	fclose(fp);
		
	printf("* Starting IO streaming (press CTRL+C to cancel)\n");
	count = 1;
	while(!stop){
	char *p_dat, *p_end;
	ptrdiff_t p_inc;
	ssize_t nbytes_rx, nbytes_tx;
	

	p_inc = iio_buffer_step(txbuf);
	p_end = iio_buffer_end(txbuf);
	j=0;
	printf("Going into for loop \n");
	for (p_dat = (char *)iio_buffer_first(txbuf,tx0_i); p_dat < p_end && stop == false; p_dat += p_inc,j++)
	{
		if(count%2==0)
		{

			if(j>=BUFF_SZ/2 && j<65528)
				((int16_t*)p_dat)[0]=(int16_t)input_wave[j%(BUFF_SZ/2)];

			else
				((int16_t*)p_dat)[0]=(int16_t)(0);

		}
		else
			((int16_t*)p_dat)[0]=(int16_t)(0);


	}
	printf("ending for loop\n");
	printf("%d\n",j);
	
	//pushing samples to DAC port
	nbytes_tx = iio_buffer_push(txbuf);
	printf("Number of bytes pushed :%d\n", (int)nbytes_tx);
	if (nbytes_tx < 0) { printf("Error pushing buf %d\n", (int) nbytes_tx); shutdown(); }
	sleep(5);
	
	count++;
	}

	shutdown();
	return 0;

}

