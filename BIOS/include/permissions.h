#ifndef BAIOS_PERMISSIONS_H
#define BAIOS_PERMISSIONS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sistema de permissões do Baios
 *
 * Cada processo possui um conjunto de capabilities.
 * O Software Microkernel valida toda requisição antes
 * de encaminhar qualquer coisa ao Hardware Microkernel.
 */

typedef enum {
    BAIOS_CAP_NONE            = 0,
    BAIOS_CAP_READ_FILE       = (1u << 0),
    BAIOS_CAP_WRITE_FILE      = (1u << 1),
    BAIOS_CAP_EXEC            = (1u << 2),
    BAIOS_CAP_NETWORK         = (1u << 3),
    BAIOS_CAP_CAMERA          = (1u << 4),
    BAIOS_CAP_MICROPHONE      = (1u << 5),
    BAIOS_CAP_LOCATION        = (1u << 6),
    BAIOS_CAP_SENSORS         = (1u << 7),
    BAIOS_CAP_BLUETOOTH       = (1u << 8),
    BAIOS_CAP_STORAGE         = (1u << 9),
    BAIOS_CAP_SYSTEM          = (1u << 10),
    BAIOS_CAP_ROOT            = (1u << 31)  /* poder total */
} baios_capability_t;

typedef u32 baios_perm_set_t;

#define BAIOS_PERM_DEFAULT_USER   (BAIOS_CAP_READ_FILE | BAIOS_CAP_WRITE_FILE | BAIOS_CAP_NETWORK)
#define BAIOS_PERM_DEFAULT_SYSTEM (BAIOS_CAP_ROOT)

typedef struct {
    baios_pid_t      pid;
    baios_perm_set_t granted;
    baios_perm_set_t requested;  /* pendente de aprovação */
} baios_perm_record_t;

baios_error_t perm_init(void);
baios_error_t perm_grant(baios_pid_t pid, baios_capability_t cap);
baios_error_t perm_revoke(baios_pid_t pid, baios_capability_t cap);
bool          perm_check(baios_pid_t pid, baios_capability_t cap);
baios_error_t perm_request(baios_pid_t pid, baios_capability_t cap);
baios_error_t perm_get(baios_pid_t pid, baios_perm_set_t *out);

/* Política: algumas capabilities só podem ser concedidas pelo sistema */
bool perm_is_dangerous(baios_capability_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_PERMISSIONS_H */
