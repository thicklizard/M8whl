/*
 * DO NOT EDIT THIS FILE
 * This file is under version control at
 *   svn://sources.blackfin.uclinux.org/toolchain/trunk/proc-defs/header-frags/
 * and can be replaced with that version at any time
 * DO NOT EDIT THIS FILE
 *
 * Copyright 2004-2011 Analog Devices Inc.
 * Licensed under the ADI BSD license.
 *   https://docs.blackfin.uclinux.org/doku.php?id=adi_bsd
 */


#ifndef _MACH_ANOMALY_H_
#define _MACH_ANOMALY_H_

#if __SILICON_REVISION__ < 3 || __SILICON_REVISION__ == 4
# error will not work on BF561 silicon version 0.0, 0.1, 0.2, or 0.4
#endif

#define ANOMALY_05000074 (1)
#define ANOMALY_05000099 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000120 (1)
#define ANOMALY_05000122 (1)
#define ANOMALY_05000127 (1)
#define ANOMALY_05000149 (1)
#define ANOMALY_05000156 (__SILICON_REVISION__ < 4)
#define ANOMALY_05000166 (1)
#define ANOMALY_05000167 (1)
#define ANOMALY_05000168 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000169 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000171 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000174 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000175 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000176 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000179 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000180 (1)
#define ANOMALY_05000181 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000182 (1)
#define ANOMALY_05000184 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000185 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000186 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000187 (1)
#define ANOMALY_05000188 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000189 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000190 (1)
#define ANOMALY_05000193 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000194 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000198 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000199 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000200 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000202 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000204 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000205 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000207 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000208 (1)
#define ANOMALY_05000209 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000215 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000219 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000220 (__SILICON_REVISION__ < 4)
#define ANOMALY_05000225 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000227 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000230 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000231 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000232 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000242 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000244 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000245 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000248 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000250 (__SILICON_REVISION__ > 2 && __SILICON_REVISION__ < 5)
#define ANOMALY_05000251 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000253 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000254 (__SILICON_REVISION__ > 3)
#define ANOMALY_05000257 (__SILICON_REVISION__ < 5 || (__SILICON_REVISION__ == 5 && CONFIG_SMP))
#define ANOMALY_05000258 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000260 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000261 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000262 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000263 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000264 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000265 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000266 (__SILICON_REVISION__ > 3)
#define ANOMALY_05000267 (1)
#define ANOMALY_05000269 (1)
#define ANOMALY_05000270 (1)
#define ANOMALY_05000272 (1)
#define ANOMALY_05000274 (1)
#define ANOMALY_05000275 (__SILICON_REVISION__ > 2)
#define ANOMALY_05000276 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000277 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000278 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000281 (__SILICON_REVISION__ <= 5)
#define ANOMALY_05000283 (1)
#define ANOMALY_05000287 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000288 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000301 (1)
#define ANOMALY_05000302 (1)
#define ANOMALY_05000305 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000307 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000310 (1)
#define ANOMALY_05000312 (1)
#define ANOMALY_05000313 (1)
#define ANOMALY_05000315 (1)
#define ANOMALY_05000320 (__SILICON_REVISION__ > 3)
#define ANOMALY_05000323 (1)
#define ANOMALY_05000326 (__SILICON_REVISION__ > 3)
#define ANOMALY_05000331 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000332 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000333 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000339 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000343 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000357 (1)
#define ANOMALY_05000362 (1)
#define ANOMALY_05000363 (__SILICON_REVISION__ < 5)
#define ANOMALY_05000366 (1)
#define ANOMALY_05000371 (1)
#define ANOMALY_05000403 (1)
#define ANOMALY_05000412 (1)
#define ANOMALY_05000416 (1)
#define ANOMALY_05000425 (1)
#define ANOMALY_05000426 (1)
#define ANOMALY_05000428 (__SILICON_REVISION__ > 3)
#define ANOMALY_05000443 (1)
#define ANOMALY_05000458 (1)
#define ANOMALY_05000461 (1)
#define ANOMALY_05000462 (1)
#define ANOMALY_05000471 (1)
#define ANOMALY_05000473 (1)
#define ANOMALY_05000475 (1)
#define ANOMALY_05000477 (1)
#define ANOMALY_05000481 (1)
#define ANOMALY_05000489 (1)
#define ANOMALY_05000491 (1)
#define ANOMALY_05000494 (1)
#define ANOMALY_05000501 (1)


#define ANOMALY_05000116 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000125 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000134 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000135 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000136 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000140 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000141 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000142 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000144 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000145 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000146 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000147 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000150 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000151 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000152 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000153 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000154 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000157 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000159 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000160 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000161 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000162 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000163 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000172 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000173 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000191 (__SILICON_REVISION__ < 3)
#define ANOMALY_05000402 (__SILICON_REVISION__ == 4)

#define ANOMALY_05000119 (0)
#define ANOMALY_05000158 (0)
#define ANOMALY_05000183 (0)
#define ANOMALY_05000233 (0)
#define ANOMALY_05000234 (0)
#define ANOMALY_05000273 (0)
#define ANOMALY_05000311 (0)
#define ANOMALY_05000353 (1)
#define ANOMALY_05000364 (0)
#define ANOMALY_05000380 (0)
#define ANOMALY_05000383 (0)
#define ANOMALY_05000386 (1)
#define ANOMALY_05000389 (0)
#define ANOMALY_05000400 (0)
#define ANOMALY_05000430 (0)
#define ANOMALY_05000432 (0)
#define ANOMALY_05000435 (0)
#define ANOMALY_05000440 (0)
#define ANOMALY_05000447 (0)
#define ANOMALY_05000448 (0)
#define ANOMALY_05000456 (0)
#define ANOMALY_05000450 (0)
#define ANOMALY_05000465 (0)
#define ANOMALY_05000467 (0)
#define ANOMALY_05000474 (0)
#define ANOMALY_05000480 (0)
#define ANOMALY_05000485 (0)

#endif
