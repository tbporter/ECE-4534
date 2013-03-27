#ifndef RELPIC_H
#define	RELPIC_H

#define FINISH 0x46
#define SLWDWN 0x38
#define SPDUPP 0x34
#define DIRRGT 0x32
#define DIRLFT 0x31
#define ERROR  0x5A

// RFID Format <stx><10-bit-address><chksum><cr><nl><etx>
char tag_finish[] = {'\x02','6','7','0','0','7','1','0','1','4','A','5','D','\xd','\xa','\x03','\0'};
// 670072BEE04B
char tag_slwdwn[] = {'\x02','6','7','0','0','7','2','B','E','E','0','4','B','\xd','\xa','\x03','\0'};
// 670072818B1F
char tag_spdupp[] = {'\x02','6','7','0','0','7','2','8','1','8','B','1','F','\xd','\xa','\x03','\0'};
// 670072C05184
char tag_dirrgt[] = {'\x02','6','7','0','0','7','2','C','0','5','1','8','4','\xd','\xa','\x03','\0'};
// 670072AF48F2
char tag_dirlft[] = {'\x02','6','7','0','0','7','2','A','F','4','8','F','2','\xd','\xa','\x03','\0'};

#endif	/* RELPIC_H */

