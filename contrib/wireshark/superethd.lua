-- SETH_dissector.lua
-- A Wireshark Lua dissector for SETH protocol over UDP (port 6081)


-- Define the SETH protocol
seth_proto = Proto("SETH", "SETH Protocol")

-- Define fields for SETH header
local fields = {
    version = ProtoField.uint8("seth.version", "Version", base.DEC),
    opt_len = ProtoField.uint8("seth.opt_len", "Option Length", base.DEC),
    oam = ProtoField.uint8("seth.oam", "OAM", base.HEX),
    critical = ProtoField.uint8("seth.critical", "Critical", base.HEX),
    reserved = ProtoField.uint8("seth.reserved", "Reserved", base.HEX),
    format = ProtoField.uint8("seth.format", "Format", base.HEX),
    channel = ProtoField.uint8("seth.channel", "Channel", base.DEC),
    sequence = ProtoField.uint32("seth.sequence", "Sequence", base.DEC),
}
fields.payload = ProtoField.bytes("seth.payload", "SETH Payload")
fields.payload_header = ProtoField.bytes("seth.payload_header", "SETH Payload Header")

fields.opt_type = ProtoField.uint8("seth.opt_type", "Type", base.DEC)
fields.opt_packet_size = ProtoField.uint16("seth.opt_packet_size", "Packet Size", base.DEC)
fields.opt_format = ProtoField.uint8("seth.opt_format", "Format", base.HEX)
fields.opt_payload_length = ProtoField.uint16("seth.opt_payload_length", "Payload Length", base.DEC)
fields.opt_part = ProtoField.uint8("seth.opt_part", "Part", base.DEC)
fields.opt_reserved = ProtoField.uint8("seth.opt_reserved", "Reserved", base.HEX)

seth_proto.fields = fields








-- Define encapsulated ethernet protocol
eth_proto = Proto("Ethernet", "SETH Ethernet Frame")


function eth_proto.dissector(buffer, pinfo, tree)
	pinfo.cols.protocol:set("Ethernet")

	local subtree = tree:add(eth_proto, buffer(), "Ethernet Payload")

	Dissector.get("eth_withoutfcs"):call(buffer(0):tvb(), pinfo, subtree)
end


function seth_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol:set("SETH")

    local subtree = tree:add(seth_proto, buffer(), "SETH Protocol Data")

    local has_opt_header = buffer(0, 1):bitfield(4,4)

    -- Add SETH header fields to the tree
    local main_header = subtree:add(seth_proto, buffer(0, 8), "Packet Header")
    main_header:add(fields.version, buffer(0, 1):bitfield(0,4))
    main_header:add(fields.opt_len, buffer(0, 1):bitfield(4,4))
    main_header:add(fields.oam, buffer(1, 1):bitfield(0,1))
    main_header:add(fields.critical, buffer(1, 1):bitfield(1,1))
    main_header:add(fields.reserved, buffer(1, 1):bitfield(2,5))
    main_header:add(fields.format, buffer(2, 1))
    main_header:add(fields.channel, buffer(3, 1))
    main_header:add(fields.sequence, buffer(4, 4))

    offset = 8
    if has_opt_header > 0 then
        local opt_type = buffer(offset, 1):uint()

        if opt_type > 0 and opt_type < 4 then
            local record_tree = main_header:add(seth_proto, buffer(offset, 4), "Opt Header: Packet Payload Header")
            record_tree:add(fields.opt_type, opt_type)
            offset = offset + 1
            record_tree:add(fields.opt_packet_size, buffer(offset, 2))
            offset = offset + 2
            record_tree:add(fields.opt_format, buffer(offset, 1))
            offset = offset + 1
            record_tree:add(fields.opt_payload_length, buffer(offset, 2))
            offset = offset + 2
            record_tree:add(fields.opt_part, buffer(offset, 1))
            local opt_sequence = buffer(offset, 1):uint()
            offset = offset + 1
            record_tree:add(fields.opt_reserved, buffer(offset, 1))
            offset = offset + 1
            -- Output the ethernet frame if this is the first in sequence
            if opt_sequence == 0 then
                eth_proto.dissector(buffer(offset):tvb(), pinfo, subtree)
            end
        end
    end


end

-- Register the dissector
local udp_table = DissectorTable.get("udp.port")
udp_table:add(58023, seth_proto)
