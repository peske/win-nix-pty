using System;
using System.Runtime.Serialization;
using JetBrains.Annotations;

namespace PtyClr
{
    public class TerminalCorruptException : Exception
    {
        internal TerminalCorruptException()
        {
        }

        internal TerminalCorruptException(string message) : base(message)
        {
        }

        internal TerminalCorruptException(string message, Exception innerException) : base(message, innerException)
        {
        }

        public TerminalCorruptException([NotNull] SerializationInfo info, StreamingContext context) : base(info,
            context)
        {
        }
    }
}