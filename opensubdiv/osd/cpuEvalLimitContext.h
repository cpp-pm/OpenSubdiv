//
//     Copyright (C) Pixar. All rights reserved.
//
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
//
#ifndef OSD_CPU_EVAL_LIMIT_CONTEXT_H
#define OSD_CPU_EVAL_LIMIT_CONTEXT_H

#include "../version.h"

#include "../osd/evalLimitContext.h"
#include "../osd/vertexDescriptor.h"

#include <map>
#include <stdio.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

class OsdCpuEvalLimitContext : public OsdEvalLimitContext {
public:
    /// \brief Factory
    /// Returns an EvalLimitContext from the given farmesh.
    /// Note : the farmesh is expected to be feature-adaptive and have ptex
    ///        coordinates tables.
    static OsdCpuEvalLimitContext * Create(FarMesh<OsdVertex> const * farmesh);

    /// Destructor
    virtual ~OsdCpuEvalLimitContext();


    /// Binds the data buffers.
    ///
    /// @param inDesc vertex buffer data descriptor shared by all input data buffers
    ///
    /// @param inQ input vertex data
    ///
    /// @param outDesc vertex buffer data descriptor shared by all output data buffers
    ///
    /// @param outQ output vertex data
    ///
    /// @param outdQu optional output derivative along "u" of the vertex data
    ///
    /// @param outdQv optional output derivative along "v" of the vertex data
    ///
    template<class VERTEX_BUFFER, class OUTPUT_BUFFER>
    void BindVertexBuffers( OsdVertexBufferDescriptor const & inDesc, VERTEX_BUFFER *inQ,
                            OsdVertexBufferDescriptor const & outDesc, OUTPUT_BUFFER *outQ, 
                                                                       OUTPUT_BUFFER *outdQu=0, 
                                                                       OUTPUT_BUFFER *outdQv=0) {
        _inDesc = inDesc;
        _inQ = inQ ? inQ->BindCpuBuffer() : 0;
 
        _outDesc = outDesc;
        _outQ   = outQ ? outQ->BindCpuBuffer() : 0 ;
        _outdQu = outdQu ? outdQu->BindCpuBuffer() : 0 ;
        _outdQv = outdQv ? outdQv->BindCpuBuffer() : 0 ;
    }

    /// Unbind the data buffers
    void UnbindVertexBuffers() {
        _inQ    = 0;
        _outQ   = 0;
        _outdQu = 0;
        _outdQv = 0;
    }

    /// Returns the input vertex buffer descriptor
    const OsdVertexBufferDescriptor & GetInputDesc() const {
        return _inDesc;
    }

    /// Returns the output vertex buffer descriptor
    const OsdVertexBufferDescriptor & GetOutputDesc() const {
        return _outDesc;
    }

    /// Returns the input vertex buffer data
    float const * GetInputVertexData() const {
        return _inQ;
    }

    /// Returns the output vertex buffer data
    float * GetOutputVertexData() const {
        return _outQ;
    }

    /// Returns the U derivative of the output vertex buffer data
    float * GetOutputVertexDataUDerivative() const {
        return _outdQu;
    }

    /// Returns the V derivative of the output vertex buffer data
    float * GetOutputVertexDataVDerivative() const {
        return _outdQv;
    }
    
    /// Returns the vector of patch arrays
    const FarPatchTables::PatchArrayVector & GetPatchArrayVector() const {
        return _patchArrays;
    }
    
    /// Returns the vector of per-patch parametric data
    const std::vector<FarPtexCoord::BitField> & GetPatchBitFields() const {
        return _patchBitFields;
    }

    /// The ordered array of control vertex indices for all the patches
    const std::vector<unsigned int> & GetControlVertices() const {
        return _patches;
    }

    /// XXXX
    const int * GetVertexValenceBuffer() const {
        return &_vertexValenceBuffer[0];
    }

    const unsigned int *GetQuadOffsetBuffer() const {
        return &_quadOffsetBuffer[0];
    }

    /// Maps coarse-face ptex coordinates to children patches
    struct PatchMap {
    public:
        
        
        /// \brief Returns the number and list of patch indices for a given face.
        ///
        /// PatchMaps map coarse faces to their childrn feature adaptive patches. 
        /// Coarse faces are indexed using their ptex face ID to resolve parametric
        /// ambiguity on non-quad faces. Note : this "map" is actually a vector, so
        /// queries are O(1) order.
        ///
        /// @param faceid the ptex face index to search for
        ///
        /// @param npatches the number of children patches found for the faceid
        ///
        /// @param patches a set of pointers to the individual patch handles
        ///
        bool GetChildPatchesHandles( int faceid, int * npatches, OsdPatchHandle const ** patches ) const {

            if (_handles.empty() or _offsets.empty() or (faceid>=(int)_offsets.size()))
                return false;
            
            *npatches = (faceid==(int)_offsets.size()-1 ? (unsigned int)_handles.size()-1 : _offsets[faceid+1]) - _offsets[faceid] + 1;
            
            *patches = &_handles[ _offsets[faceid] ];
            
            return true;
        }
    
    private:
    
        friend class OsdCpuEvalLimitContext;
               
        typedef std::multimap<unsigned int, OsdPatchHandle> MultiMap;
        
        // Constructor
        PatchMap( FarPatchTables const & patchTables ) {
                  
            int npatches = (int)patchTables.GetNumPatches();
            
            _handles.reserve(npatches);
            
            MultiMap mmap;

            
            FarPatchTables::PatchArrayVector const & patchArrays =
                patchTables.GetPatchArrayVector();

            FarPatchTables::PtexCoordinateTable const & ptxTable =
                patchTables.GetPtexCoordinatesTable();

            int nfaces =0;
                           
            assert( not ptxTable.empty() );
                
            for (int arrayid = 0; arrayid < (int)patchArrays.size(); ++arrayid) {

                FarPatchTables::PatchArray const & pa = patchArrays[arrayid];

                 int ringsize = pa.GetDescriptor().GetNumControlVertices();

                 for (unsigned int j=0; j < pa.GetNumPatches(); ++j) {

                    int faceId = ptxTable[pa.GetPatchIndex()+j].faceIndex;

                    OsdPatchHandle handle = { arrayid, j*ringsize, (unsigned int)mmap.size() };

                    mmap.insert( std::pair<unsigned int, OsdPatchHandle>(faceId, handle));

                    nfaces = std::max(nfaces, faceId);
                }
            }
            ++nfaces;

            _handles.resize( mmap.size() );
            
            _offsets.reserve( nfaces );
            _offsets.push_back(0);

            unsigned int handlesIdx = 0, faceId=mmap.begin()->first;

            // Serialize the multi-map
            for (MultiMap::const_iterator it=mmap.begin(); it!=mmap.end(); ++it, ++handlesIdx) {
                        
                assert(it->first >= faceId);
                
                if (it->first != faceId) {

                    faceId = it->first;
                    
                    // position the offset marker to the new face                    
                    _offsets.push_back( handlesIdx );
                }
                
                // copy the patch id into the table
                _handles[handlesIdx] = it->second;
            }
        }

        // Patch handle allowing location of individual patch data inside patch
        // arrays or in serialized form
        std::vector<OsdPatchHandle> _handles;
        
        // offset to the first handle of the child patches for each coarse face
        std::vector<unsigned int> _offsets; 
    };

    /// Returns a map object that can connect a faceId to a list of children patches
    const PatchMap * GetPatchesMap() const {
        return _patchMap;
    }

protected:
    explicit OsdCpuEvalLimitContext(FarMesh<OsdVertex> const * farmesh);

private:

    // Topology data for a mesh
    FarPatchTables::PatchArrayVector    _patchArrays;    // patch descriptor for each patch in the mesh
    FarPatchTables::PTable              _patches;        // patch control vertices
    std::vector<FarPtexCoord::BitField> _patchBitFields; // per-patch parametric info
    
    FarPatchTables::VertexValenceTable   _vertexValenceBuffer; // extra Gregory patch data buffers
    FarPatchTables::QuadOffsetTable      _quadOffsetBuffer;

    PatchMap * _patchMap;

    OsdVertexBufferDescriptor _inDesc,
                              _outDesc;
    
    float * _inQ,      // input vertex data
          * _outQ,     // output vertex data
          * _outdQu,   // U derivative of output vertex data
          * _outdQv;   // V derivative of output vertex data
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OSD_CPU_EVAL_LIMIT_CONTEXT_H */
