#ifndef __WEB_INPUT_C__
#define __WEB_INPUT_C__

const char INFO_TABLE[] = \
	"<table border=1>"
	" <caption> Info </caption>"
	"   <tr>"
	"    <td>State</td>"
	"    <td> %s </td>"
	"    <td id=spacer />"
	"    <td>Loop?</td>"
	"    <td><input type=checkbox name=loop value=1 %s></td>"
	"   </tr>"
	"   <tr>"
	"    <td>Cur Speed</td>"
	"    <td> %d cm/s </td>"
	"    <td id=spacer />"
	"    <td>Avg  Speed</td>"
	"    <td> %d cm/s </td>"
	"   </tr>"
	"   <tr>"
	"    <td>Lap</td>"
	"    <td> %d </td>"
	"    <td id=spacer />"
	"    <td>Finished?</td>"
	"    <td> %d </td>"
	"   </tr>"
	"   <tr>"
	"    <td>Amps</td>"
	"    <td> %d mA </td>"
	"   </tr>"
	"</table>";

const char MOTOR_TABLE[] = \
	"<td style=height:225px;width:50px;>"
	" <meter id=motorControlTop min=63 max=127 value=%u></meter>"
	" <meter id=motorControlBottom min=0 max=65 value=%u></meter>"
	"</td>"
	"<td style=height:225px;width:50px>"
	" <meter id=motorControlTop min=63 max=127 value=%u></meter>"
	" <meter id=motorControlBottom min=1 max=65 value=%u></meter>"
	"</td>";

#endif