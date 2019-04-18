#include <asf.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "conf_board.h"
#include "conf_example.h"
#include "conf_uart_serial.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"

#include "logo.h"
#include "lavagens.h"
#include "previous.h"
#include "next.h"
#include "pause.h"
#include "play.h"
#include "enxague.h"
#include "fast.h"
#include "daily.h"
#include "centrifuge.h"
#include "custom.h"
#include "rotation.h"
#include "unlocked.h"
#include "weight.h"


#include "maquina1.h"

#define MAX_ENTRIES        3
#define STRING_LENGTH     70

#define USART_TX_MAX_LENGTH     0xff


struct ili9488_opt_t g_ili9488_display_opt;
const uint32_t BUTTON_W = 120;
const uint32_t BUTTON_H = 150;
const uint32_t BUTTON_BORDER = 2;
const uint32_t BUTTON_X = ILI9488_LCD_WIDTH / 2;
const uint32_t BUTTON_Y = ILI9488_LCD_HEIGHT / 2;


t_ciclo *initMenuOrder() {

	c_rapido.previous = &c_enxague;

	c_rapido.next = &c_diario;



	c_diario.previous = &c_rapido;

	c_diario.next = &c_pesado;



	c_pesado.previous = &c_diario;

	c_pesado.next = &c_enxague;



	c_enxague.previous = &c_pesado;

	c_enxague.next = &c_centrifuga;



	c_centrifuga.previous = &c_enxague;

	c_centrifuga.next = &c_custom;
	
	c_custom.previous = &c_centrifuga;
	c_custom.next = &c_rapido;



	return(&c_diario);

}


static void configure_lcd(void) {
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
}


void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while (*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if (letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}
}


char * toArray(int number)
{
	int n = log10(number) + 1;
	int i;
	char *numberArray = calloc(n, sizeof(char));
	for (i = 0; i < n; ++i, number /= 10)
	{
		numberArray[i] = number % 10;
	}
	return numberArray;
}


static void mxt_init(struct mxt_device *device)
{
	enum status_code status;

	/* T8 configuration object data */
	uint8_t t8_object[] = {
		0x0d, 0x00, 0x05, 0x0a, 0x4b, 0x00, 0x00,
		0x00, 0x32, 0x19
	};

	/* T9 configuration object data */
	uint8_t t9_object[] = {
		0x8B, 0x00, 0x00, 0x0E, 0x08, 0x00, 0x80,
		0x32, 0x05, 0x02, 0x0A, 0x03, 0x03, 0x20,
		0x02, 0x0F, 0x0F, 0x0A, 0x00, 0x00, 0x00,
		0x00, 0x18, 0x18, 0x20, 0x20, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x02,
		0x02
	};

	/* T46 configuration object data */
	uint8_t t46_object[] = {
		0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x03,
		0x00, 0x00
	};

	/* T56 configuration object data */
	uint8_t t56_object[] = {
		0x02, 0x00, 0x01, 0x18, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	/* TWI configuration */
	twihs_master_options_t twi_opt = {
		.speed = MXT_TWI_SPEED,
		.chip = MAXTOUCH_TWI_ADDRESS,
	};

	status = (enum status_code)twihs_master_setup(MAXTOUCH_TWI_INTERFACE, &twi_opt);
	Assert(status == STATUS_OK);

	/* Initialize the maXTouch device */
	status = mxt_init_device(device, MAXTOUCH_TWI_INTERFACE,
	MAXTOUCH_TWI_ADDRESS, MAXTOUCH_XPRO_CHG_PIO);
	Assert(status == STATUS_OK);

	/* Issue soft reset of maXTouch device by writing a non-zero value to
	* the reset register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
	MXT_GEN_COMMANDPROCESSOR_T6, 0)
	+ MXT_GEN_COMMANDPROCESSOR_RESET, 0x01);

	/* Wait for the reset of the device to complete */
	delay_ms(MXT_RESET_TIME);

	/* Write data to configuration registers in T7 configuration object */
	mxt_write_config_reg(device, mxt_get_object_address(device,
	MXT_GEN_POWERCONFIG_T7, 0) + 0, 0x20);
	mxt_write_config_reg(device, mxt_get_object_address(device,
	MXT_GEN_POWERCONFIG_T7, 0) + 1, 0x10);
	mxt_write_config_reg(device, mxt_get_object_address(device,
	MXT_GEN_POWERCONFIG_T7, 0) + 2, 0x4b);
	mxt_write_config_reg(device, mxt_get_object_address(device,
	MXT_GEN_POWERCONFIG_T7, 0) + 3, 0x84);

	/* Write predefined configuration data to configuration objects */
	mxt_write_config_object(device, mxt_get_object_address(device,
	MXT_GEN_ACQUISITIONCONFIG_T8, 0), &t8_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
	MXT_TOUCH_MULTITOUCHSCREEN_T9, 0), &t9_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
	MXT_SPT_CTE_CONFIGURATION_T46, 0), &t46_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
	MXT_PROCI_SHIELDLESS_T56, 0), &t56_object);

	/* Issue recalibration command to maXTouch device by writing a non-zero
	* value to the calibrate register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
	MXT_GEN_COMMANDPROCESSOR_T6, 0)
	+ MXT_GEN_COMMANDPROCESSOR_CALIBRATE, 0x01);
}

void draw_screen(void) {
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH - 1, ILI9488_LCD_HEIGHT - 1);
}

uint32_t convert_axis_system_x(uint32_t touch_y) {
	// entrada: 4096 - 0 (sistema de coordenadas atual)
	// saida: 0 - 320
	return ILI9488_LCD_WIDTH - ILI9488_LCD_WIDTH * touch_y / 4096;
}

uint32_t convert_axis_system_y(uint32_t touch_x) {
	// entrada: 0 - 4096 (sistema de coordenadas atual)
	// saida: 0 - 320
	return ILI9488_LCD_HEIGHT * touch_x / 4096;
}

typedef struct {
	int32_t flag;
	uint32_t x_location;
	uint32_t y_location;
	uint32_t height;
	uint32_t width;
	ili9488_color_t* image[];
} botao;

//480x320
//flag | x | y | altura | largura | endereço da imagem
botao b1 = { 1,0,0,80,320, &lavagensImage };
botao b2 = { 1,0,80,80,107, &previous };
botao b3 = { 1,107,80,80,106, &play };
botao b4 = { 1,213,80,80,107, &next };
botao b5 = { 1,0,160,160,160, &next }; //pesado
botao b6 = { 1,160,160,160,160, &next }; //bolhas
botao b7 = { 1,0,320,160,160, &next }; //rpm
botao b8 = { 1,160,320,160,160, &next }; //tempo total
botao* botoes[] = { &b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8};


const int num_botoes = 8;

volatile Bool f_rtt_alarme = false;

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);


int check_button_click(uint32_t tx, uint32_t ty, botao* but) {
	
	ili9488_draw_pixmap(but->x_location, but->y_location, but->height, but->width, but->image);
	if (tx >= but->x_location && tx <= but->x_location + but->width) {
		if (ty >= but->y_location && ty <= but->y_location + but->height) {
			but->flag = -1 * (but->flag);
			if (but->flag > 0) {
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_ORANGE));
				ili9488_draw_filled_rectangle(but->x_location, but->y_location, but->x_location + but->width, but->y_location + but->height);
			}
			else {
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
				ili9488_draw_filled_rectangle(but->x_location, but->y_location, but->x_location + but->width, but->y_location + but->height);
			}

		}
	}
}

void mxt_debounce(struct mxt_device *device)
{
	/* USART tx buffer initialized to 0 */
	char tx_buf[STRING_LENGTH * MAX_ENTRIES] = {0};
	uint8_t i = 0; /* Iterator */

	/* Temporary touch event data struct */
	struct mxt_touch_event touch_event;

	/* Collect touch events and put the data in a string,
	 * maximum 2 events at the time */
	do {
		/* Temporary buffer for each new touch event line */
		char buf[STRING_LENGTH];
	
		/* Read next touch event in the queue, discard if read fails */
		if (mxt_read_touch_event(device, &touch_event) != STATUS_OK) {
			continue;
		}
		
		 // eixos trocados (quando na vertical LCD)
		uint32_t conv_x = convert_axis_system_x(touch_event.y);
		uint32_t conv_y = convert_axis_system_y(touch_event.x);
		

		/* Add the new string to the string buffer */
		strcat(tx_buf, buf);
		i++;

		/* Check if there is still messages in the queue and
		 * if we have reached the maximum numbers of events */
	} while ((mxt_is_message_pending(device)) & (i < MAX_ENTRIES));

}


void mxt_handler(struct mxt_device *device)
{
	/* USART tx buffer initialized to 0 */
	char tx_buf[STRING_LENGTH * MAX_ENTRIES] = { 0 };
	uint8_t i = 0; /* Iterator */

	/* Temporary touch event data struct */
	struct mxt_touch_event touch_event;

	/* Collect touch events and put the data in a string,
	* maximum 2 events at the time */
	do {
		/* Temporary buffer for each new touch event line */
		char buf[STRING_LENGTH];

		/* Read next next touch event in the queue, discard if read fails */
		if (mxt_read_touch_event(device, &touch_event) != STATUS_OK) {
			continue;
		}

		// eixos trocados (quando na vertical LCD)
		uint32_t conv_x = convert_axis_system_x(touch_event.y);
		uint32_t conv_y = convert_axis_system_y(touch_event.x);

		/* Format a new entry in the data string that will be sent over USART */
		sprintf(buf, "Nr: %1d, X:%4d, Y:%4d, Status:0x%2x conv X:%3d Y:%3d\n\r",
		touch_event.id, touch_event.x, touch_event.y,
		touch_event.status, conv_x, conv_y);
		//update_screen(conv_x, conv_y);
		for (int ii = 0; ii < num_botoes; ii += 1) {



			check_button_click(conv_x, conv_y, botoes[ii]);
		}


		/* Add the new string to the string buffer */
		strcat(tx_buf, buf);
		i++;

		/* Check if there is still messages in the queue and
		* if we have reached the maximum numbers of events */
	} while ((mxt_is_message_pending(device)) & (i < MAX_ENTRIES));

	/* If there is any entries in the buffer, send them over USART */
	if (i > 0) {
		usart_serial_write_packet(USART_SERIAL_EXAMPLE, (uint8_t *)tx_buf, strlen(tx_buf));
	}
}


void RTT_Handler(void)

{

	uint32_t ul_status;



	/* Get RTT status */

	ul_status = rtt_get_status(RTT);



	/* IRQ due to Time has changed */

	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {}



	/* IRQ due to Alarm */

	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {



		f_rtt_alarme = true;                  // flag RTT alarme

	}

}


static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)

{

	uint32_t ul_previous_time;



	/* Configure RTT for a 1 second tick interrupt */

	rtt_sel_source(RTT, false);

	rtt_init(RTT, pllPreScale);



	ul_previous_time = rtt_read_timer_value(RTT);

	while (ul_previous_time == rtt_read_timer_value(RTT));



	rtt_write_alarm_time(RTT, IrqNPulses + ul_previous_time);



	/* Enable RTT interrupt */

	NVIC_DisableIRQ(RTT_IRQn);

	NVIC_ClearPendingIRQ(RTT_IRQn);

	NVIC_SetPriority(RTT_IRQn, 0);

	NVIC_EnableIRQ(RTT_IRQn);

	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);

}


void update_screen(t_ciclo *p_primeiro){
	if (botoes[0]->flag > 0) {
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
		//draw_screen(); //desperdicio de tempo
		ili9488_draw_filled_rectangle(botoes[0]->x_location, botoes[0]->y_location, botoes[0]->x_location + botoes[0]->width, botoes[0]->y_location + botoes[0]->height);
		font_draw_text(&calibri_36, p_primeiro->nome, 20, 20, 1);
		ili9488_draw_filled_rectangle(botoes[4]->x_location, botoes[4]->y_location, botoes[4]->x_location + botoes[4]->width, botoes[4]->y_location + botoes[4]->height);
		font_draw_text(&calibri_36, (p_primeiro->heavy == 1 ? "pes:SIM" : "pes:NAO"), botoes[4]->x_location + 20, botoes[4]->y_location + 20, 1);
		ili9488_draw_filled_rectangle(botoes[5]->x_location, botoes[5]->y_location, botoes[5]->x_location + botoes[5]->width, botoes[5]->y_location + botoes[5]->height);
		font_draw_text(&calibri_36, (p_primeiro->bubblesOn == 1 ? "bol:SIM" : "bol:NAO"), botoes[5]->x_location + 20, botoes[5]->y_location + 20, 1);
		char RPM[12];
		sprintf(RPM, "%d", p_primeiro->centrifugacaoRPM);
		ili9488_draw_filled_rectangle(botoes[6]->x_location, botoes[6]->y_location, botoes[6]->x_location + botoes[6]->width, botoes[6]->y_location + botoes[6]->height);
		font_draw_text(&calibri_36, "RPM:", botoes[6]->x_location + 20, botoes[6]->y_location + 20, 1);
		font_draw_text(&calibri_36, RPM, botoes[6]->x_location + 20, botoes[6]->y_location + 20 + 36, 1);
		if (p_primeiro->heavy == 1) {
			char tempo[12];
			sprintf(tempo, "cent:%d", p_primeiro->centrifugacaoTempo);
			ili9488_draw_filled_rectangle(botoes[7]->x_location, botoes[7]->y_location, botoes[7]->x_location + botoes[7]->width, botoes[7]->y_location + botoes[7]->height);
			font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20, 1);

			sprintf(tempo, "enxa:%d", p_primeiro->enxagueTempo);
			font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20 + 36, 1); // + 36 sempre -> \n
			sprintf(tempo, "totP:%d", (p_primeiro->enxagueTempo + p_primeiro->centrifugacaoTempo) * 2);
			font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20 + (36 * 2), 1); // + 36 sempre -> \n

		}
		else {
			char tempo[12];
			sprintf(tempo, "cent:%d", p_primeiro->centrifugacaoTempo);
			ili9488_draw_filled_rectangle(botoes[7]->x_location, botoes[7]->y_location, botoes[7]->x_location + botoes[7]->width, botoes[7]->y_location + botoes[7]->height);
			font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20, 1);

			sprintf(tempo, "enxa:%d", p_primeiro->enxagueTempo);
			font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20 + 36, 1); // + 36 sempre -> \n
			sprintf(tempo, "tot:%d", p_primeiro->enxagueTempo + p_primeiro->centrifugacaoTempo);
			font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20 + (36 * 2), 1); // + 36 sempre -> \n

		}

	}
	
	
}



int main(void)
{
	struct mxt_device device; /* Device data container */

	/* Initialize the USART configuration struct */
	const usart_serial_options_t usart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};

	sysclk_init(); /* Initialize system clocks */
	board_init();  /* Initialize board */
	configure_lcd();
	draw_screen();
	/* Initialize the mXT touch device */
	mxt_init(&device);

	/* Initialize stdio on USART */
	stdio_serial_init(USART_SERIAL_EXAMPLE, &usart_serial_options);

	printf("\n\rmaXTouch data USART transmitter\n\r");

	ili9488_draw_pixmap(0, 50, 319, 129, &logoImage);

	delay_ms(1000);

	ili9488_draw_pixmap(0, 150, 80, 80, &next);

	delay_ms(1000);

	draw_screen();

	t_ciclo *p_primeiro = initMenuOrder();
	font_draw_text(&calibri_36, "SIM", botoes[4]->x_location, botoes[4]->y_location, 1);

	int started = 0;
	unsigned long long ftime = 0;
	unsigned long long cctime = 0;
	unsigned int porta_aberta = 0;
	f_rtt_alarme=true;

	update_screen(p_primeiro);
	
	while (true) {
		/* Check for any pending messages and run message handler if any
		* message is found in the queue */
		if (mxt_is_message_pending(&device)) {
			mxt_handler(&device);
			delay_ms(200);
			mxt_debounce(&device);
		}
		
		if (p_primeiro->id==5){
			if (1==1)
			if (botoes[4]->flag>0){
				
				if(p_primeiro->heavy) {
					p_primeiro->heavy=!(p_primeiro->heavy);
				} else {
					p_primeiro->heavy=!(p_primeiro->heavy);
				}
				botoes[4]->flag=-1;
				update_screen(p_primeiro);
				
			}
			if (botoes[5]->flag>0){
				
				if(p_primeiro->bubblesOn) {
					p_primeiro->bubblesOn=!(p_primeiro->bubblesOn);
					} else {
					p_primeiro->bubblesOn=!(p_primeiro->bubblesOn);
				}
				botoes[5]->flag=-1;
				update_screen(p_primeiro);
				
			}
			if (botoes[6]->flag>0){
				p_primeiro->centrifugacaoRPM+=100;
				p_primeiro->centrifugacaoRPM%=1300;
				botoes[6]->flag=-1;
				update_screen(p_primeiro);
				
			}
		}
		
		
		//TODO: fazer handle de porta aberta

		if ((ftime > cctime || botoes[2]->flag < 0) && !(porta_aberta)) {
			if (botoes[2]->flag < 0) botoes[2]->flag = 1;
			if (started == 0) {
				started = 1;
				ftime = (p_primeiro->enxagueTempo + p_primeiro->centrifugacaoTempo) * 60;
				ili9488_draw_filled_rectangle(botoes[7]->x_location, botoes[7]->y_location, botoes[7]->x_location + botoes[7]->width, botoes[7]->y_location + botoes[7]->height);
				cctime = 0;
			}
			else {
				if (f_rtt_alarme) {
					uint16_t pllPreScale = (int)(((float)32768) / 2.0);
					cctime+=2;
					uint32_t irqRTTvalue = 4;
					RTT_init(pllPreScale, irqRTTvalue);
					char tempo[12];
					sprintf(tempo, "/%d", ftime);
					font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20 + (36 * 2), 1); // + 36 sempre -> \n
					font_draw_text(&calibri_36, "tempo: ", botoes[7]->x_location + 20, botoes[7]->y_location + 20 + (36 * 0), 1); // + 36 sempre -> \n
					sprintf(tempo, "%d", cctime);
					font_draw_text(&calibri_36, tempo, botoes[7]->x_location + 20, botoes[7]->y_location + 20 + (36 * 1), 1); // + 36 sempre -> \n
					f_rtt_alarme = false;
					
				}
				if (cctime >= ftime || botoes[2]->flag < 0) {
					started = 0;
					cctime=0;
					ftime=0;
				}
				

			}


		}
		else {
			if (botoes[0]->flag > 0)font_draw_text(&calibri_36, p_primeiro->nome, 20, 20, 1);
			if (botoes[1]->flag > 0) {
				botoes[1]->flag = -1;
				p_primeiro = p_primeiro->previous;
				update_screen(p_primeiro);

			}
			if (botoes[3]->flag > 0) {
				botoes[3]->flag = -1;
				p_primeiro = p_primeiro->next;
				update_screen(p_primeiro);

			}



		}
	}

	return 0;
}
