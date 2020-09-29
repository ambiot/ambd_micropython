/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Daniel Campora
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EXCEPTION_H_
#define EXCEPTION_H_
/*****************************************************************************
 *                              External variables
 * ***************************************************************************/
extern int  user_interrupt_char;

extern const char mpexception_value_invalid_arguments[];
extern const char mpexception_num_type_invalid_arguments[];
extern const char mpexception_uncaught[];

// OS 
extern const char mpexception_os_resource_not_avaliable[];
extern const char mpexception_os_operation_failed[];
extern const char mpexception_os_request_not_possible[];

// WIFI
extern const char mpexception_socket_settimeout_failed[];
extern const char mpexception_socket_recv_failed[];
extern const char mpexception_socket_timeout[];
/*****************************************************************************
 *                              External Functions
 * ***************************************************************************/
void mpexception_init0 (void);

#endif /* MPEXCEPTION_H_ */
