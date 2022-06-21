# AddTwoInts_RequestEvent

# Indicates this request event was emitted from a client
uint8 CLIENT_SENT      = 0

# Indicates this request event was emitted from a service
uint8 SERVICE_RECEIVED = 1

# Whether this is a CLIENT_SENT or SERVICE_RECEIVED event.
uint8 request_type

# Timestamp for when the event occurred (sent or received time)
builtin_interfaces/Time stamp

# Unique identifier for the client that sent the request
# Note, this is only unique for the current session
unique_identifier_msgs/UUID client_id

# Sequence number for the request
# Combined with the client ID, this creates a unique ID for the request
int64 sequence_number

# The actual request content sent or received
# Setting this field is optional
# example_interfaces/AddTwoInts__Request request
AddTwoIntsMockRequest request
