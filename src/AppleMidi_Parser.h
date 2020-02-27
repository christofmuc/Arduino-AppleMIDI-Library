#pragma once

#include "utility/Deque.h"
#include "utility/endian.h"

#include "AppleMidi_Defs.h"

#include "AppleMidi_Settings.h"

#include "AppleMidi_Namespace.h"

BEGIN_APPLEMIDI_NAMESPACE

template <class UdpClass, class Settings>
class AppleMidiSession;

template <class UdpClass, class Settings>
class AppleMIDIParser
{
public:
	AppleMidiSession<UdpClass, Settings> *session;

	parserReturn parse(Deque<byte, Settings::MaxBufferSize> &buffer, const amPortType &portType)
	{
		conversionBuffer cb;

		V_DEBUG_PRINT("AppleMIDI_Parser::Parser received ");
		V_DEBUG_PRINT(buffer.size());
		V_DEBUG_PRINTLN(" bytes");

		byte signature[2]; // Signature "Magic Value" for Apple network MIDI session establishment
		byte command[2];   // 16-bit command identifier (two ASCII characters, first in high 8 bits, second in low 8 bits)

		size_t minimumLen = (sizeof(signature) + sizeof(command)); // Signature + Command
		if (buffer.size() < minimumLen)
            return parserReturn::NotSureGiveMeMoreData;

		size_t i = 0;

		signature[0] = buffer[i++];
		signature[1] = buffer[i++];
		if (0 != memcmp(signature, amSignature, sizeof(amSignature)))
		{
			// E_DEBUG_PRINT("Wrong signature: 0x");
			// E_DEBUG_PRINT(signature[0], HEX);
			// E_DEBUG_PRINT(signature[1], HEX);
			// E_DEBUG_PRINTLN(" was expecting 0xFFFF");

            return parserReturn::UnexpectedData;
		}

		command[0] = buffer[i++];
		command[1] = buffer[i++];

		if (false)
		{
		}
#ifdef APPLEMIDI_LISTENER
		else if (0 == memcmp(command, amInvitation, sizeof(amInvitation)))
		{
			V_DEBUG_PRINTLN("received Invitation");

			byte protocolVersion[4];

			minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
			if (buffer.size() < minimumLen)
			{
                return parserReturn::NotEnoughData;
			}

			// 2 (stored in network byte order (big-endian))
			protocolVersion[0] = buffer[i++];
			protocolVersion[1] = buffer[i++];
			protocolVersion[2] = buffer[i++];
			protocolVersion[3] = buffer[i++];
			if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
			{
				// T_DEBUG_PRINT("Wrong protocolVersion: 0x");
				// T_DEBUG_PRINT(protocolVersion[0], HEX);
				// T_DEBUG_PRINT(protocolVersion[1], HEX);
				// T_DEBUG_PRINT(protocolVersion[2], HEX);
				// T_DEBUG_PRINT(protocolVersion[3], HEX);
				// T_DEBUG_PRINTLN(" was expecting 0x00000002");
                return parserReturn::UnexpectedData;
			}

			AppleMIDI_Invitation invitation;

			// A random number generated by the session's APPLEMIDI_INITIATOR.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			invitation.initiatorToken = ntohl(cb.value32);
			
			// The sender's synchronization source identifier.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			invitation.ssrc = ntohl(cb.value32);

			V_DEBUG_PRINT("initiator: 0x");
			V_DEBUG_PRINT(invitation.initiatorToken, HEX);
			V_DEBUG_PRINT(" ssrc: 0x");
			V_DEBUG_PRINTLN(invitation.ssrc, HEX);

#ifdef KEEP_SESSION_NAME
            uint16_t bi = 0;
            while ((i < buffer.size()) && (buffer[i] != 0x00))
            {
                if (bi <= APPLEMIDI_SESSION_NAME_MAX_LEN)
                    invitation.sessionName[bi++] = buffer[i];
                i++;
            }
            invitation.sessionName[bi++] = '\0';
#else
            while ((i < buffer.size()) && (buffer[i] != 0x00))
                i++;
#endif
			if (i == buffer.size() || buffer[i++] != 0x00)
                return parserReturn::NotEnoughData;

            V_DEBUG_PRINT("AppleMidi Consumed ");
            V_DEBUG_PRINT(i);
            V_DEBUG_PRINTLN(" bytes");

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedInvitation(invitation, portType);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amEndSession, sizeof(amEndSession)))
		{
			V_DEBUG_PRINTLN("received EndSession");

			byte protocolVersion[4];

			minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
			if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

			// 2 (stored in network byte order (big-endian))
			protocolVersion[0] = buffer[i++];
			protocolVersion[1] = buffer[i++];
			protocolVersion[2] = buffer[i++];
			protocolVersion[3] = buffer[i++];
			if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
			{
				// V_DEBUG_PRINT("Wrong protocolVersion: 0x");
				// V_DEBUG_PRINT(protocolVersion[0], HEX);
				// V_DEBUG_PRINT(protocolVersion[1], HEX);
				// V_DEBUG_PRINT(protocolVersion[2], HEX);
				// V_DEBUG_PRINT(protocolVersion[3], HEX);
				// V_DEBUG_PRINTLN(" was expecting 0x00000002");
                return parserReturn::UnexpectedData;
			}

			AppleMIDI_EndSession endSession;

			// A random number generated by the session's APPLEMIDI_INITIATOR.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			endSession.initiatorToken = ntohl(cb.value32);
            
			// The sender's synchronization source identifier.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			endSession.ssrc = ntohl(cb.value32);

            V_DEBUG_PRINT("AppleMidi Consumed ");
            V_DEBUG_PRINT(i);
            V_DEBUG_PRINTLN(" bytes");

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedEndSession(endSession);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amSynchronization, sizeof(amSynchronization)))
		{
			V_DEBUG_PRINTLN("received Syncronization");

			AppleMIDI_Synchronization synchronization;

			// minimum amount : 4 bytes for sender SSRC, 1 byte for count, 3 bytes padding and 3 times 8 bytes
			minimumLen += (4 + 1 + 3 + (3 * 8));
			if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

			// The sender's synchronization source identifier.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			synchronization.ssrc = ntohl(cb.value32);

			synchronization.count = buffer[i++];
			buffer[i++];
			buffer[i++];
			buffer[i++]; // padding, unused
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			cb.buffer[4] = buffer[i++];
			cb.buffer[5] = buffer[i++];
			cb.buffer[6] = buffer[i++];
			cb.buffer[7] = buffer[i++];
			synchronization.timestamps[0] = ntohll(cb.value64);

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			cb.buffer[4] = buffer[i++];
			cb.buffer[5] = buffer[i++];
			cb.buffer[6] = buffer[i++];
			cb.buffer[7] = buffer[i++];
			synchronization.timestamps[1] = ntohll(cb.value64);

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			cb.buffer[4] = buffer[i++];
			cb.buffer[5] = buffer[i++];
			cb.buffer[6] = buffer[i++];
			cb.buffer[7] = buffer[i++];
			synchronization.timestamps[2] = ntohll(cb.value64);

            V_DEBUG_PRINT("AppleMidi Consumed ");
            V_DEBUG_PRINT(i);
            V_DEBUG_PRINTLN(" bytes");

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedSynchronization(synchronization);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amReceiverFeedback, sizeof(amReceiverFeedback)))
		{
			//V_DEBUG_PRINTLN("received ReceiverFeedback");

			AppleMIDI_ReceiverFeedback receiverFeedback;

			minimumLen += (sizeof(receiverFeedback.ssrc) + sizeof(receiverFeedback.sequenceNr) + sizeof(receiverFeedback.dummy));
			if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			receiverFeedback.ssrc = ntohl(cb.value32);

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			receiverFeedback.sequenceNr = ntohs(cb.value16);
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			receiverFeedback.dummy = ntohs(cb.value16);

			V_DEBUG_PRINT("ssrc: 0x");
			V_DEBUG_PRINTLN(receiverFeedback.ssrc, HEX);
			V_DEBUG_PRINT("sequenceNr: ");
			V_DEBUG_PRINTLN(receiverFeedback.sequenceNr);

            V_DEBUG_PRINT("AppleMidi Consumed ");
            V_DEBUG_PRINT(i);
            V_DEBUG_PRINTLN(" bytes");

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedReceiverFeedback(receiverFeedback);

            return parserReturn::Processed;
		}
#endif
#ifdef APPLEMIDI_INITIATOR
		else if (0 == memcmp(command, amInvitationAccepted, sizeof(amInvitationAccepted)))
		{
            V_DEBUG_PRINTLN("received Invitation Accepted");

            byte protocolVersion[4];

            minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
            if (buffer.size() < minimumLen)
            {
                return parserReturn::NotEnoughData;
            }

            // 2 (stored in network byte order (big-endian))
            protocolVersion[0] = buffer[i++];
            protocolVersion[1] = buffer[i++];
            protocolVersion[2] = buffer[i++];
            protocolVersion[3] = buffer[i++];
            if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
            {
                // T_DEBUG_PRINT("Wrong protocolVersion: 0x");
                // T_DEBUG_PRINT(protocolVersion[0], HEX);
                // T_DEBUG_PRINT(protocolVersion[1], HEX);
                // T_DEBUG_PRINT(protocolVersion[2], HEX);
                // T_DEBUG_PRINT(protocolVersion[3], HEX);
                // T_DEBUG_PRINTLN(" was expecting 0x00000002");
                return parserReturn::UnexpectedData;
            }

            AppleMIDI_Invitation invitationAccepted;

            // A random number generated by the session's APPLEMIDI_INITIATOR.
            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            invitationAccepted.initiatorToken = ntohl(cb.value32);
            
            // The sender's synchronization source identifier.
            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            invitationAccepted.ssrc = ntohl(cb.value32);

            V_DEBUG_PRINT("initiator: 0x");
            V_DEBUG_PRINT(invitationAccepted.initiatorToken, HEX);
            V_DEBUG_PRINT(" ssrc: 0x");
            V_DEBUG_PRINTLN(invitationAccepted.ssrc, HEX);

#ifdef KEEP_SESSION_NAME
            uint16_t bi = 0;
            while ((i < buffer.size()) && (buffer[i] != 0x00))
            {
                if (bi <= APPLEMIDI_SESSION_NAME_MAX_LEN)
                    invitationAccepted.sessionName[bi++] = buffer[i];
                i++;
            }
            invitationAccepted.sessionName[bi++] = '\0';
#else
            while ((i < buffer.size()) && (buffer[i] != 0x00))
                i++;
#endif
            if (i == buffer.size() || buffer[i++] != 0x00)
                return parserReturn::NotEnoughData;

            V_DEBUG_PRINT("AppleMidi Consumed ");
            V_DEBUG_PRINT(i);
            V_DEBUG_PRINTLN(" bytes");

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

            session->ReceivedInvitationAccepted(invitationAccepted, portType);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amInvitationRejected, sizeof(amInvitationRejected)))
		{
            return parserReturn::Processed;
		}
        else if (0 == memcmp(command, amBitrateReceiveLimit, sizeof(amBitrateReceiveLimit)))
        {
            return parserReturn::Processed;
        }
#endif
        return parserReturn::UnexpectedData;
	}
};

END_APPLEMIDI_NAMESPACE
