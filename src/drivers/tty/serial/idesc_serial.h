/**
 * @file
 *
 * @date Dec 18, 2013
 * @author: Anton Bondarev
 */

#ifndef IDESC_SERIAL_H_
#define IDESC_SERIAL_H_

struct idesc_ops;

extern const struct idesc_ops *idesc_serial_get_ops(void);

#endif /* IDESC_SERIAL_H_ */
