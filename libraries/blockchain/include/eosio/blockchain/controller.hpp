#pragma once
#include <eosio/blockchain/block.hpp>

namespace eosio { namespace blockchain {

   /**
    * @class controller
    *
    * This controller coordinates the asynchrounous process of deriving
    * the current chain state, logging blocks to disk, etc.
    *
    * All API calls are thread safe and may be called by any thread. Signals may dispatch their
    * handlers on any thread.
    */
   class controller {
      public:
         typedef std::function<void( const transaction_metadata_ptr& ) > completion_handler;

         controller();
         ~controller();

         database& db();

         /**
          *  In a properly functioning network this list should be less than 2/3 the number of producers, and if
          *  they are utilizing proper advance confirmation then this list should be at most 2-3 long.
          */
         struct chain_status {
            vector<block_id_type> pending; /// first one is last irreversible, last one is head block / height
         };

         chain_status get_chain_status()const;


         /**
          * Performs lookup of an existing known transaction.
          */
         transaction_metadata_ptr lookup_transaction( transaction_id_type id );

         /**
          * Looks up a block by id and will call callback() with the block_metadata_ptr once it is found,
          * if it is not found then it will call the callback with a nullptr.
          */
         void  lookup_block( block_id_type, function<void(block_metadata_ptr)> callback );

         /**
          * Looks up a range of blocks by sequence number. The purpose of this interface is to
          * facilitate effecient batch reads of the block log for synchronizing nodes.
          */
         void  lookup_block( block_num_type first, 
                             block_num_type last, 
                             function<void(vector<block_metadata_ptr>)> callback );

         /**
          *  When a new transaction is received from any source this method will attempt
          *  to add it to our internal state of known transactions. If the transaction is
          *  already known then nothing will happen, otherwise the various signals will be
          *  emitted as the transaction is processed.
          */
         void add_transaction( vector<char>&& packed_transaction,
                               transaction_id_type trx_id, 
                               signed_transaction_ptr = signed_transaction_ptr(), 
                               set<public_key_type> signatures = set<public_key_type>() );

         /**
          * Given a block summary, this method will reconstitute the block_data from the
          * known transactions after first validating that it links to known forks.
          */
         void add_block( vector<char>&&   packed_summary,
                         block_id_type    block_id,
                         block_summary_ptr summary = block_summary_ptr() );

         /**
          * This method adds a full block which contains all the transactions in the block, used for syncing,
          * we can skip signature verification of transactions if this block is older than last irreversible block.
          */
         void add_full_block( vector<char>& packed_block_data,
                              block_id_type block_id,
                              block_data_ptr full_block = block_data_ptr() );

         /**
          * Calling this method will cause the current "pending state" to be converted into a block
          * and the block_validated() signal to be generated. This process will occur at the given time
          * assuming the account_name is a valid producer and the producer_key matches.  This method should
          * be called 100 ms before the `when` so that the scheduler can factor in the need to finalize the
          * block for production and ensure all in-process transactions complete their execution. 
          *
          * A call to add_block() that extends the current block will cancel the pending producer_block.
          *
          * When must be a multiple of the block interval and at a time in the future, but it must not be more
          * than 1 block interval in the future.
          */
         void produce_block( time_point when, account_name producer, private_key_type producer_key );

         //void add_pre_commit( ... );
         //void add_pre_commit_receipt( ... );
         //void add_commit( ... );

         /**
          *  
          */
         ///{
         
         /// emitted when a new transaction is discovered but before it is validated
         signal<void(transaction_metadata_ptr)> transaction_added;
         /// emitted when a new transaction has been executed and applied to pending state
         signal<void(transaction_metadata_ptr)> transaction_validated;
         signal<void(transaction_metadata_ptr)> transaction_confirmed;
         signal<void(transaction_metadata_ptr)> transaction_rejected; /// perminenent rejection (expired or invalid)

         signal<void(block_metadata_ptr)>       block_linked; /// block linked, but not validated
         signal<void(block_metadata_ptr)>       block_validated; /// block linked and validated
         signal<void(block_metadata_ptr)>       block_confirmed; /// declared immutable 

         /*
         signal<void(precommit)>                new_precommit;
         signal<void(precommit_receipt)>        new_precommit_receipt;
         signal<void(commit)>                   new_commit;
         */
         ///}

      private:
         io_service&         _ios;
         unique_ptr<strand>  _strand;

         /// DATABASE
             // auth scope
             // per account scope
             // block scope
             
         /// FORKDB (track what fork each producer should be on based on messages received)
         /// BLOCK LOG (committed blocks)

   };

} } /// eosio::blockchain
