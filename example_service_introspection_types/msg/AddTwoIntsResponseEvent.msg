# AddTwoInts_ResponseEvent

# Indicates this response event was emitted from a service
uint8 SERVICE_SENT     = 0

# Indicates this response event was emitted from a client
uint8 CLIENT_RECEIVED  = 1

# Whether this is a SERVICE_SENT or CLIENT_RECEIVED event.
uint8 response_type

# Timestamp for when the event occurred (sent or received time)
builtin_interfaces/Time stamp

# Unique identifier for the client that sent the request associated with this response
# Note, this is only unique for the current session
unique_identifier_msgs/UUID client_id

# Sequence number for the request associated with this response
# Combined with the client ID, this creates a unique ID for the request
int64 sequence_number

# The actual response content sent or received
# Setting this field is optional
# example_interfaces/AddTwoInts_Response response
AddTwoIntsMockResponse response
