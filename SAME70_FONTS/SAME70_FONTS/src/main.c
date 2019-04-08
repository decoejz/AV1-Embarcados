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

#define YEAR        2019
#define MOUNTH      4
#define DAY         8
#define WEEK        2
#define HOUR        15
#define MINUTE      5
#define SECONDS     0

volatile alarm_sec = SECONDS+2;
volatile alarm_min = MINUTE;
volatile alarm_hour = HOUR;

struct ili9488_opt_t g_ili9488_display_opt;

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
		
		if (alarm_min==60){
			alarm_hour += 1;
			alarm_min = 0;
			alarm_sec = 1;
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			rtc_set_time_alarm(RTC, 1, alarm_hour, 1, alarm_min, 1, alarm_sec);
		}
		else if(alarm_sec==60){
			alarm_sec = 1;
			alarm_min += 1;
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			rtc_set_time_alarm(RTC, 1, alarm_hour, 1, alarm_min, 1, alarm_sec);
		}
		else{
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			rtc_set_time_alarm(RTC, 1, alarm_hour, 1, alarm_min, 1, alarm_sec);
			alarm_sec += 1;
		}
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

void print_time(void){
	char hour_s[32];
	char min_s[32];
	char sec_s[32];
	
	int *now_hour, *now_minute, *now_sec;
	
	rtc_get_time(RTC,now_hour,now_minute,now_sec);
	
	/*int specific_h = HOUR - alarm_hour;//*now_hour;
	int specific_m = MINUTE - alarm_min;//*now_minute;
	int specific_s = SECONDS - alarm_sec;//*now_sec;*/
	
	int specific_h = alarm_hour - HOUR;
	int specific_m = alarm_min - MINUTE;
	int specific_s = alarm_sec - SECONDS;
	
	sprintf(hour_s,"%d",specific_h);
	sprintf(min_s,"%d",specific_m);
	sprintf(sec_s,"%d",specific_s);
	
	font_draw_text(&arial_72, hour_s, 15, 75, 1);	
	font_draw_text(&arial_72, min_s, 115, 75, 1);	
	font_draw_text(&arial_72, sec_s, 215, 75, 1);
}

int main(void) {
	board_init();
	sysclk_init();	
	configure_lcd();
	
	RTC_init();
	
	rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
	rtc_set_time_alarm(RTC, 1, alarm_hour, 1, alarm_min, 1, SECONDS+1);
	
	font_draw_text(&calibri_36, "Tempo Decorrido", 15, 15, 1);
	
	
	font_draw_text(&arial_72, "00", 15, 75, 1);	
	//font_draw_text(&calibri_36, ":", 110, 75, 1);
	font_draw_text(&arial_72, "00", 115, 75, 1);
	//font_draw_text(&calibri_36, ":", 210, 75, 1);		
	font_draw_text(&arial_72, "00", 215, 75, 1);
	
	font_draw_text(&calibri_36, "Velocidade", 15, 170, 1);
	
	font_draw_text(&arial_72, "00", 15, 230, 1);
	font_draw_text(&calibri_36, "km/h", 115, 230, 1);
	
	font_draw_text(&calibri_36, "Distancia", 15, 345, 1);
	
	font_draw_text(&arial_72, "00", 15, 400, 1);
	font_draw_text(&calibri_36, "m", 115, 400, 1);
	
	//font_draw_text(&sourcecodepro_28, "OIMUNDO", 50, 50, 1);
	//font_draw_text(&arial_72, "102456", 50, 200, 2);
	while(1) {
		print_time();
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}