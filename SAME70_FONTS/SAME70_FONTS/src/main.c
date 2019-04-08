/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"

#define YEAR        0
#define MOUNTH      0
#define DAY         0
#define WEEK        0
#define HOUR        0
#define MINUTE      0
#define SECONDS     0

#define BUT_PIO_ID			  ID_PIOA
#define BUT_PIO				  PIOA
#define BUT_PIN				  19
#define BUT_PIN_MASK			  (1 << BUT_PIN)

#define LED_PIO       PIOC
#define LED_PIO_ID    ID_PIOC
#define LED_IDX       8u
#define LED_IDX_MASK  (1u << LED_IDX)

struct ili9488_opt_t g_ili9488_display_opt;

volatile Bool f_rtt_alarme = false;

volatile pulsos = 0;
volatile float distancia_total = 0;

void print_time(void);
void count_vel(int now_time);
void pin_toggle(Pio *pio, uint32_t mask);


void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}

void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}	
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/*
	*  Verifica por qual motivo entrou
	*  na interrupcao, se foi por segundo
	*  ou Alarm
	*/
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
		print_time();
				
		int now_hour, now_minute, now_sec;
		rtc_get_time(RTC,&now_hour,&now_minute,&now_sec);

		/*if (now_minute==60){
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			rtc_set_time_alarm(RTC, 1, now_hour, 1, now_minute, 1, now_sec+1);
		}
		else if(now_sec==60){
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			rtc_set_time_alarm(RTC, 1, now_hour, 1, now_minute, 1, now_sec+1);
		}
		else{*/
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			rtc_set_time_alarm(RTC, 1, now_hour, 1, now_minute, 1, now_sec+1);
		//}
	}
	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);	
}

void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(RTC, YEAR, MOUNTH, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECONDS);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC,  RTC_IER_ALREN);
}

void Button0_Handler(){
	pulsos += 1;
}

void BUT_init(void){
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT_PIO_ID);
	
	pio_set_input(BUT_PIO, BUT_PIN_MASK, PIO_PULLUP | PIO_DEBOUNCE);

	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrupção */
	pio_enable_interrupt(BUT_PIO, BUT_PIN_MASK);
	
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIN_MASK, PIO_IT_FALL_EDGE, Button0_Handler);

	/* habilita interrupçcão do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);
};

void io_init(void){
	/* led */
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
}

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		pin_toggle(LED_PIO, LED_IDX_MASK);
		f_rtt_alarme = true;                 // flag RTT alarme
		pulsos = 0;
	}
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}

void print_time(void){
	char hour_s[32];
	char min_s[32];
	char sec_s[32];
	
	int now_hour, now_minute, now_sec;
	
	rtc_get_time(RTC,&now_hour,&now_minute,&now_sec);
	
	sprintf(hour_s,"%d:%d:%d",now_hour,now_minute,now_sec);
	
	font_draw_text(&calibri_36, hour_s, 15, 75, 1);
}

void count_vel(int now_time){
	float w = 2*3.14*pulsos/now_time;
	float vel = w*0.325*3.6;
	
	char velocidade[32];
	
	sprintf(velocidade,"%f km/h",vel);
	font_draw_text(&calibri_36, velocidade, 15, 230, 1);
	pulsos = 0;
}

void distancia(void){
	distancia_total += 2*3.14*0.325*pulsos;
	
	char distancia[32];
	
	sprintf(distancia,"%f m",distancia_total);
	font_draw_text(&calibri_36, distancia, 15, 400, 1);
}

int main(void) {
	board_init();
	sysclk_init();	
	configure_lcd();
	
	RTC_init();
	BUT_init();
	io_init();
	
	f_rtt_alarme = true;
	
	int now_hour, now_minute, now_sec;
	rtc_get_time(RTC,&now_hour,&now_minute,&now_sec);
	
	rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
	rtc_set_time_alarm(RTC, 1, now_hour, 1, now_minute, 1, now_sec+1);
	
	font_draw_text(&calibri_36, "Tempo Decorrido", 15, 15, 1);
	
	font_draw_text(&calibri_36, "Velocidade", 15, 170, 1);
	
	font_draw_text(&calibri_36, "00 km/h", 15, 230, 1);
	
	font_draw_text(&calibri_36, "Distancia", 15, 345, 1);
	
	font_draw_text(&calibri_36, "00 m", 15, 400, 1);
	
	while(1) {
		
		if (f_rtt_alarme){
		  uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
		  uint32_t irqRTTvalue  = 4;
     
		  RTT_init(pllPreScale, irqRTTvalue); 
		  distancia();       
		  count_vel(irqRTTvalue);
		  f_rtt_alarme = false;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}