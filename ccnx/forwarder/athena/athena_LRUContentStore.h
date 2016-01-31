/*
 * Copyright (c) 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Patent rights are not granted under this agreement. Patent rights are
 *       available under FRAND terms.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @author Alan Walendowski, Palo Alto Research Center (Xerox PARC)
 * @copyright 2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
 */

#ifndef libathena_LRUContentStore
#define libathena_LRUContentStore
#include <stdbool.h>

#include <parc/algol/parc_JSON.h>
#include <parc/algol/parc_HashCode.h>

#include <ccnx/forwarder/athena/athena_ContentStore.h>

struct AthenaLRUContentStore;
typedef struct AthenaLRUContentStore AthenaLRUContentStore;

typedef struct AthenaLRUContentStoreConfig {
    size_t capacityInMB;
} AthenaLRUContentStoreConfig;

/**
 * Increase the number of references to a `AthenaLRUContentStore` instance.
 *
 * Note that new `AthenaLRUContentStore` is not created,
 * only that the given `AthenaLRUContentStore` reference count is incremented.
 * Discard the reference by invoking `athenaLRUContentStore_Release`.
 *
 * @param [in] instance A pointer to a valid AthenaLRUContentStore instance.
 *
 * @return The same value as @p instance.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *
 *     AthenaLRUContentStore *b = athenaLRUContentStore_Acquire();
 *
 *     athenaLRUContentStore_Release(&a);
 *     athenaLRUContentStore_Release(&b);
 * }
 * @endcode
 */
AthenaLRUContentStore *athenaLRUContentStore_Acquire(const AthenaLRUContentStore *instance);

#ifdef Libccnx_DISABLE_VALIDATION
#  define athenaLRUContentStore_OptionalAssertValid(_instance_)
#else
#  define athenaLRUContentStore_OptionalAssertValid(_instance_) athenaLRUContentStore_AssertValid(_instance_)
#endif

/**
 * Assert that the given `AthenaLRUContentStore` instance is valid.
 *
 * @param [in] instance A pointer to a valid AthenaLRUContentStore instance.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *
 *     athenaLRUContentStore_AssertValid(a);
 *
 *     printf("Instance is valid.\n");
 *
 *     athenaLRUContentStore_Release(&b);
 * }
 * @endcode
 */
void athenaLRUContentStore_AssertValid(const AthenaLRUContentStore *instance);

/**
 * Print a human readable representation of the given `AthenaLRUContentStore`.
 *
 * @param [in] instance A pointer to a valid AthenaLRUContentStore instance.
 * @param [in] indentation The indentation level to use for printing.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *
 *     athenaLRUContentStore_Display(a, 0);
 *
 *     athenaLRUContentStore_Release(&a);
 * }
 * @endcode
 */
void athenaLRUContentStore_Display(const AthenaContentStoreImplementation *store, int indentation);

/**
 * Determine if two `AthenaLRUContentStore` instances are equal.
 *
 * The following equivalence relations on non-null `AthenaLRUContentStore` instances are maintained: *
 *   * It is reflexive: for any non-null reference value x, `athenaLRUContentStore_Equals(x, x)` must return true.
 *
 *   * It is symmetric: for any non-null reference values x and y, `athenaLRUContentStore_Equals(x, y)` must return true if and only if
 *        `athenaLRUContentStore_Equals(y x)` returns true.
 *
 *   * It is transitive: for any non-null reference values x, y, and z, if
 *        `athenaLRUContentStore_Equals(x, y)` returns true and
 *        `athenaLRUContentStore_Equals(y, z)` returns true,
 *        then `athenaLRUContentStore_Equals(x, z)` must return true.
 *
 *   * It is consistent: for any non-null reference values x and y, multiple invocations of `athenaLRUContentStore_Equals(x, y)`
 *         consistently return true or consistently return false.
 *
 *   * For any non-null reference value x, `athenaLRUContentStore_Equals(x, NULL)` must return false.
 *
 * @param [in] x A pointer to a valid AthenaLRUContentStore instance.
 * @param [in] y A pointer to a valid AthenaLRUContentStore instance.
 *
 * @return true The instances x and y are equal.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *     AthenaLRUContentStore *b = athenaLRUContentStore_Create();
 *
 *     if (athenaLRUContentStore_Equals(a, b)) {
 *         printf("Instances are equal.\n");
 *     }
 *
 *     athenaLRUContentStore_Release(&a);
 *     athenaLRUContentStore_Release(&b);
 * }
 * @endcode
 * @see athenaLRUContentStore_HashCode
 */
bool athenaLRUContentStore_Equals(const AthenaLRUContentStore *x, const AthenaLRUContentStore *y);

/**
 * Returns a hash code value for the given instance.
 *
 * The general contract of `HashCode` is:
 *
 * Whenever it is invoked on the same instance more than once during an execution of an application,
 * the `HashCode` function must consistently return the same value,
 * provided no information used in a corresponding comparisons on the instance is modified.
 *
 * This value need not remain consistent from one execution of an application to another execution of the same application.
 * If two instances are equal according to the {@link athenaLRUContentStore_Equals} method,
 * then calling the {@link athenaLRUContentStore_HashCode} method on each of the two instances must produce the same integer result.
 *
 * It is not required that if two instances are unequal according to the
 * {@link athenaLRUContentStore_Equals} function,
 * then calling the `athenaLRUContentStore_HashCode`
 * method on each of the two objects must produce distinct integer results.
 *
 * @param [in] instance A pointer to a valid AthenaLRUContentStore instance.
 *
 * @return The hashcode for the given instance.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *
 *     PARCHashCode hashValue = athenaLRUContentStore_HashCode(buffer);
 *     athenaLRUContentStore_Release(&a);
 * }
 * @endcode
 */
PARCHashCode athenaLRUContentStore_HashCode(const AthenaLRUContentStore *instance);

/**
 * Determine if an instance of `AthenaLRUContentStore` is valid.
 *
 * Valid means the internal state of the type is consistent with its required current or future behaviour.
 * This may include the validation of internal instances of types.
 *
 * @param [in] instance A pointer to a valid AthenaLRUContentStore instance.
 *
 * @return true The instance is valid.
 * @return false The instance is not valid.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *
 *     if (athenaLRUContentStore_IsValid(a)) {
 *         printf("Instance is valid.\n");
 *     }
 *
 *     athenaLRUContentStore_Release(&a);
 * }
 * @endcode
 *
 */
bool athenaLRUContentStore_IsValid(const AthenaLRUContentStore *instance);

/**
 * Produce a null-terminated string representation of the specified `AthenaLRUContentStore`.
 *
 * The result must be freed by the caller via {@link parcMemory_Deallocate}.
 *
 * @param [in] instance A pointer to a valid AthenaLRUContentStore instance.
 *
 * @return NULL Cannot allocate memory.
 * @return non-NULL A pointer to an allocated, null-terminated C string that must be deallocated via {@link parcMemory_Deallocate}.
 *
 * Example:
 * @code
 * {
 *     AthenaLRUContentStore *a = athenaLRUContentStore_Create();
 *
 *     char *string = athenaLRUContentStore_ToString(a);
 *
 *     athenaLRUContentStore_Release(&a);
 *
 *     parcMemory_Deallocate(&string);
 * }
 * @endcode
 *
 * @see athenaLRUContentStore_Display
 */
char *athenaLRUContentStore_ToString(const AthenaLRUContentStore *instance);

extern AthenaContentStoreInterface AthenaContentStore_LRUImplementation;
#endif
