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
   " <tr><td>Loop?</td><td><input type=checkbox name=loop value=1 %s></td></tr>"
   " <tr><td>Finished?</td><td> %d </td></tr>"
   "</table>";

const char INPUT_FORM[] = \
   "<tr><td>START \t<input type=checkbox name=start %s></td></tr>"
   "<tr><td>M4_Demo \t<input type=checkbox name=M4Demo %s></td></tr>"
   "<tr><td>Show Nav Msgs \t<input type=checkbox name=printNav %s></td></tr>"
   "<tr><td>Show ZigBee Msgs \t<input type=checkbox name=printZigBee %s></td></tr>";

const char MOTOR_TABLE[] = \
	"<td>"
	" <meter class='motorControl motorControlTop' min=63 max=127 value=%u></meter>"
	" <meter class='motorControl motorControlBottom' min=0 max=65 value=%u></meter>"
	"</td>"
	"<td>"
	" <meter class='motorControl motorControlTop' min=63 max=127 value=%u></meter>"
	" <meter class='motorControl motorControlBottom' min=1 max=65 value=%u></meter>"
	"</td>";

#endif