#ifndef SENDERS_RECEIVERS_H
#define SENDERS_RECEIVERS_H
#include <string>
#include "headers/enums_types.h"

/*!
 * \brief sendResponse sends RESPONSE_T over the network
 * \param fd file descriptor associated with the connection
 * \param resp response to send
 * \return zero on success
 */
int64_t sendResponse(int64_t fd, RESPONSE_T &resp);


/*!
 * \brief sendAdrressStruct sends sockaddr_storage over the network
 * \param fd file descriptor associated with the connection
 * \param st structure to be sent
 * \return zero on success
 */
int64_t sendAdrressStruct(int64_t fd, const struct sockaddr_storage &st);

/*!
 * \brief sendInt64 sends int64_t over the network connection
 * \param fd file descriptor associated with the connection
 * \param i value to send
 * \return zero on success
 */
int64_t sendInt64(int64_t fd, int64_t i);

/*!
 * \brief sendString sends string over the network connection
 * \param fd file descriptor associated with the connection
 * \param str string to be sent
 * \return zero on success
 */
int64_t sendString(int64_t fd, std::string str);

/*!
 * \brief sendFile sends file over the network connection
 * \param fd file descriptor associated with the connection
 * \param str path to the file to be sent
 * \return zero on success
 * handles all problems connected with sending the file
 */
int64_t sendFile(int64_t fd, std::string str);

/*!
 * \brief sendCmd sends command over the network connection
 * \param fd file descriptor associated with the connection
 * \param cmd Command to be sent
 * \return zero on success
 */
int64_t sendCmd(int64_t fd, CMDS cmd);

/*!
 * \brief receiveResponse receives response from the network connection
 * \param fd file descriptor associated with the open connection
 * \param resp field to receive
 * \return zero on success
 */
int64_t receiveResponse(int64_t fd, RESPONSE_T &resp) ;

/*!
 * \brief receiveAddressStruct receives struct with address from the network connection
 * \param fd file descriptor associated with the open connection
 * \param struct field to receive
 * \return zero on success
 */
int64_t receiveAddressStruct(int64_t fd, struct sockaddr_storage &st);

/*!
 * \brief receiveInt64( receives integer from the network connection
 * \param fd file descriptor associated with the open connection
 * \param i field to receive
 * \return zero on success
 */
int64_t receiveInt64(int64_t fd, int64_t &i);

/*!
 * \brief receiveFile receives file from the network connection
 * \param fd file descriptor associated with the open connection
 * \param fn path where to save the received file
 * \return zero on success
 */
int64_t receiveFile(int64_t fd, std::string fn);

/*!
 * \brief receiveString receives string from the network connection
 * \param fd file descriptor associated with the open connection
 * \return received string
 */
std::string receiveString(int64_t fd);

#endif // SENDERS_RECEIVERS_H
