#ifndef __WEB_INPUT_C__
#define __WEB_INPUT_C__

const char INFO_TABLE[] = \
   "<table id=info class=left border=1>"
   " <caption>Info</caption>"
   " <tr><td>State</td><td> %s </td></tr>"
   " <tr><td>Cur Speed</td><td> %d cm/s </td></tr>"
   " <tr><td>Avg  Speed</td><td> %d cm/s </td></tr>"
   " <tr><td>Lap</td><td> %d </td></tr>"
   " <tr><td>Amps</td><td> %d mA </td></tr>"
   " <tr><td>Finished?</td><td> %d </td></tr>"
   "</table>";

const char INPUT_FORM[] = \
   "<tr><td>START	<input type=checkbox name=start %s></td></tr>"
   "<tr><td>Loop?	<input type=checkbox name=loop %s></td></tr>"
   "<tr><td>M4_Demo	<input type=checkbox name=M4Demo %s></td></tr>"
   "<tr><td>Show Nav Msgs	<input type=checkbox name=printNav %s></td></tr>"
   "<tr><td>Show ZigBee Msgs	<input type=checkbox name=printZigBee %s></td></tr>";

const char MOTOR_TABLE[] = \
	"<td>"
	" <meter class='motorControl motorControlTop' min=63 max=127 value=%u></meter>"
	" <meter class='motorControl motorControlBottom' min=0 max=65 value=%u></meter>"
	"</td>"
	"<td>"
	" <meter class='motorControl motorControlTop' min=63 max=127 value=%u></meter>"
	" <meter class='motorControl motorControlBottom' min=1 max=65 value=%u></meter>"
	"</td>";

const char IR_DIST_TABLE[] = \
	"<tr>"
	"<td>%d</td><td>%d</td><td>%d</td><td>%d</td>"
	"</tr>"
	"<tr>"
	"<td>%d</td><td></td><td></td><td>%d</td>"
	"</tr>";

const char SENSOR_TABLE[] = \
	"<tr>"
	"<td>Encoders</td><td>%d</td><td>%d</td>"
	"</tr></table><br><table border=1>"
	"<tr><td></td><th>Go Left</th><th>Go Right</th><th>Go Fast</th><th>Go Slow</th><th>Finish</th><th>Error</th></tr>"
	"<tr><td>RFID</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>";

const char TIMES_TABLE[] = \
  "<tr><td>Nominal Time</td><td>%02u:%02u.%03u</td></tr>"
  "<tr><td>Actual Time</td><td>%02u:%02u.%03u</td></tr>";

#endif